#include <errno.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef enum { PRODUCER = 1,
			   CONSUMER = 2 } client_type;

time_t          arrival_time[ 1024 ];
ITEM**          buf;
int             bufsize, count = 0;
int             clients = 0, prod = 0, cons = 0;
int             prod_served = 0, cons_served = 0;
int             client_max_reject = 0, prod_max_reject = 0, cons_max_reject = 0;
int             slow_clients = 0;
pthread_mutex_t buf_mutex, client_mutex;
sem_t           full, empty;

ITEM* makeItem( uint32_t len )
{
	ITEM* i         = malloc( sizeof( ITEM ) );
	i->letters      = malloc( len * sizeof( char ) );
	i->letters[ 0 ] = '\0';
	i->size         = len;
	return i;
}


void exit_client( client_type type )
{
	pthread_mutex_lock( &client_mutex );

	fprintf( stderr, "Exiting client %s, clients:%d, prod:%d, cons:%d\n", type == 1 ? "PROD" : "CONS", clients, prod, cons );

	clients--;
	switch ( type ) {
		case PRODUCER:
			prod--;
			break;
		case CONSUMER:
			cons--;
			break;
		default:
			break;
	}
	pthread_mutex_unlock( &client_mutex );
}

void* producer( void* arg )
{
	int ssock = *( int* ) arg;
	sem_wait( &empty );

	if ( write( ssock, "GO\r\n", 4 ) < 0 ) {
		fprintf( stderr, "could not write to PRODUCER\n" );
		close( ssock );
		exit_client( PRODUCER );
		pthread_exit( NULL );
	}

	int ilen;

	if ( read( ssock, &ilen, sizeof( ilen ) ) < 0 ) {
		fprintf( stderr, "could not read item len\n" );
		close( ssock );
		exit_client( PRODUCER );
		pthread_exit( NULL );
	}


	ilen = ntohl( ilen );

	ITEM* i = makeItem( ilen );
	i->psd  = ssock;

	pthread_mutex_lock( &buf_mutex );

	buf[ count ] = i;
	count++;
	prod_served++;

	pthread_mutex_unlock( &buf_mutex );

	sem_post( &full );
	fprintf( stderr, "Succefully placed producer item\n" );
}

void* consumer( void* arg )
{
	int ssock = *( int* ) arg;
	sem_wait( &full );
	pthread_mutex_lock( &buf_mutex );

	ITEM* i          = buf[ count - 1 ];
	buf[ count - 1 ] = NULL;
	count--;
	cons_served++;
	pthread_mutex_unlock( &buf_mutex );
	sem_post( &empty );

	int size = htonl( i->size );

	if ( write( ssock, &size, 4 ) < 0 ) {
		fprintf( stderr, "could not send item len\n" );
		close( ssock );
		exit_client( CONSUMER );
		pthread_exit( 0 );
	}

	if ( write( i->psd, "GO\r\n", 4 ) < 0 ) {
		fprintf( stderr, "could not send GO to producer\n" );
		close( ssock );
		close( i->psd );
		exit_client( CONSUMER );
		pthread_exit( 0 );
	}

	char* buf = malloc( sizeof( char ) * BUFSIZE );
	int   cc, rb = 0;
	do {

		if ( ( cc = read( i->psd, buf, BUFSIZE ) ) < 0 ) {
			fprintf( stderr, "could not read from producer\n" );
			break;
		}

		rb += cc;
		if ( write( ssock, buf, cc ) < 0 ) {
			fprintf( stderr, "could not write to consumer\n" );
			break;
		}

	} while ( cc != 0 && rb < i->size );

	write( i->psd, "DONE\r\n", 7 );
	close( ssock );
	close( i->psd );
	fprintf( stderr, "Succefully sent item to consumer\n" );
	exit_client( CONSUMER );
	exit_client( PRODUCER );
}

bool is_free( client_type type )
{
	switch ( type ) {
		case PRODUCER:
			return prod < MAX_PROD;
		case CONSUMER:
			return cons < MAX_CON;
	}
}

void conn_handler( int ssock )
{
	char*     charbuf = malloc( 18 * sizeof( char ) );
	pthread_t thread;
	int       cc;

	if ( ( cc = read( ssock, charbuf, 18 ) ) <= 0 ) {
		fprintf( stderr, "Could not read type of client from socket: %s\n", strerror( errno ) );
		exit_client( 0 );
		return;
	}

	charbuf[ cc ] = '\0';
	int* sock     = malloc( sizeof( int ) );
	*sock         = ssock;

	if ( strncmp( "PRODUCE\r\n", charbuf, 9 ) == 0 ) {

		pthread_mutex_lock( &client_mutex );


		bool free = is_free( PRODUCER );
		prod += free;
		pthread_mutex_unlock( &client_mutex );


		if ( free ) {
			pthread_create( &thread, NULL, producer, ( void* ) sock );
		} else {
			prod_max_reject++;
			close( ssock );
			exit_client( 0 );
			fprintf( stderr, "MAX PROD REJECT\n" );
		}

	} else if ( strncmp( "CONSUME\r\n", charbuf, 9 ) == 0 ) {

		pthread_mutex_lock( &client_mutex );
		bool free = is_free( CONSUMER );
		cons += free;
		pthread_mutex_unlock( &client_mutex );


		if ( free ) {
			pthread_create( &thread, NULL, consumer, ( void* ) sock );
		} else {
			cons_max_reject++;
			close( ssock );
			exit_client( 0 );
			fprintf( stderr, "MAX CONS REJECT\n" );
		}

	} else if ( strncmp( charbuf, "STATUS/", 7 ) == 0 ) {
		fprintf( stderr, "New status client has arrived\n" );
		charbuf             = charbuf + 7;
		char responce[ 12 ] = { 0 };

		if ( !strncmp( charbuf, "CURR", 4 ) ) {
			charbuf = charbuf + 4;
			if ( !strncmp( charbuf, "CLI", 3 ) ) {
				sprintf( responce, "%d\r\n", clients );
			} else if ( !strncmp( charbuf, "PROD", 4 ) ) {
				sprintf( responce, "%d\r\n", prod );
			} else if ( !strncmp( charbuf, "CONS", 4 ) ) {
				sprintf( responce, "%d\r\n", cons );
			} else {
				fprintf( stderr, "Unrecongnized status command\n" );
				exit_client( 0 );
				close( ssock );
				return;
			}
		} else if ( !strncmp( charbuf, "TOT", 3 ) ) {
			charbuf = charbuf + 3;
			if ( !strncmp( charbuf, "PROD", 4 ) ) {
				sprintf( responce, "%d\r\n", prod_served );
			} else if ( !strncmp( charbuf, "CONS", 4 ) ) {
				sprintf( responce, "%d\r\n", cons_served );
			} else {
				fprintf( stderr, "Unrecongnized status command\n" );
				exit_client( 0 );
				close( ssock );
				return;
			}
		} else if ( !strncmp( charbuf, "REJ", 3 ) ) {
			charbuf = charbuf + 3;

			if ( !strncmp( charbuf, "MAX", 3 ) ) {
				sprintf( responce, "%d\r\n", client_max_reject );
			} else if ( !strncmp( charbuf, "PROD", 4 ) ) {
				sprintf( responce, "%d\r\n", prod_max_reject );
			} else if ( !strncmp( charbuf, "CONS", 4 ) ) {
				sprintf( responce, "%d\r\n", cons_max_reject );
			} else if ( !strncmp( charbuf, "SLOW", 4 ) ) {
				sprintf( responce, "%d\r\n", slow_clients );
			} else {
				fprintf( stderr, "Unrecongnized status command\n" );
				exit_client( 0 );
				close( ssock );
				return;
			}
		} else {
			fprintf( stderr, "Unrecongnized status command\n" );
			exit_client( 0 );
			close( ssock );
			return;
		}

		write( ssock, responce, strlen( responce ) );
		exit_client( 0 );
		close( ssock );
	} else {
		fprintf( stderr, "Could not identify client\n" );
		exit_client( 0 );
		close( ssock );
	}
}

int main( int argc, char** argv )
{
	char*              service;
	int                msock;
	int                rport = 0;
	socklen_t          alen;
	struct sockaddr_in fsin;
	volatile int       ssock;
	pthread_t          thread;
	fd_set             rfds;
	fd_set             afds;
	int                fd;
	int                nfds;
	int                cc;
	switch ( argc ) {
		case 2:
			// No args? let the OS choose a port and tell the user
			bufsize = atoi( argv[ 1 ] );
			rport   = 1;
			break;
		case 3:
			// User provides a port? then use it
			bufsize = atoi( argv[ 2 ] );
			service = argv[ 1 ];
			break;
		default:
			fprintf( stderr, "usage: server [port] bufsize\n" );
			exit( -1 );
	}


	buf = malloc( bufsize * sizeof( ITEM* ) );

	pthread_mutex_init( &client_mutex, NULL );
	pthread_mutex_init( &buf_mutex, NULL );
	sem_init( &full, 0, 0 );
	sem_init( &empty, 0, bufsize );

	msock = passivesock( service, "tcp", QLEN, &rport );
	if ( rport ) {
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );
		fflush( stdout );
	}

	nfds = msock + 1;
	FD_ZERO( &afds );
	FD_SET( msock, &afds );

	struct rlimit lim = { 1023, 1023 };
	setrlimit( RLIMIT_NOFILE, &lim );

	for ( ;; ) {
		struct timeval timeout;

		timeout.tv_sec  = REJECT_TIME;
		timeout.tv_usec = 0;

		memcpy( ( char* ) &rfds, ( char* ) &afds, sizeof( rfds ) );

		int status = select( nfds, &rfds, ( fd_set* ) 0, ( fd_set* ) 0,
							 &timeout );

		// fprintf(stderr, "Out of select %d fd_setsize: %d max socket:%d\n", status, FD_SETSIZE, nfds);
		if ( status < 0 ) {
			fprintf( stderr, "Server select error: %s\n", strerror( errno ) );
			exit( -1 );
		}

		if ( FD_ISSET( msock, &rfds ) ) {
			// we can call accept with no fear of blocking
			alen      = sizeof( fsin );
			int ssock = accept( msock, ( struct sockaddr* ) &fsin, &alen );
			if ( ssock < 0 ) {
				fprintf( stderr, "accept: %s\n", strerror( errno ) );
				exit( -1 );
			}

			pthread_mutex_lock( &client_mutex );
			bool free = clients < MAX_CLIENTS;
			clients += free;
			pthread_mutex_unlock( &client_mutex );


			if ( free ) {
				// If a new client arrives, we must add it to our afds set
				FD_SET( ssock, &afds );
				arrival_time[ ssock ] = time( NULL );
				// and increase the maximum, if necessary
				if ( ssock + 1 > nfds )
					nfds = ssock + 1;
				fprintf( stderr, "Client has arrived on %d\n", ssock );
			} else {
				client_max_reject++;
				close( ssock );
			}
		}

		//check for timed out sockets
		for ( int fd = 0; fd < nfds; fd++ ) {
			if ( time( NULL ) - arrival_time[ fd ] > REJECT_TIME && arrival_time[ fd ] > 0 ) {
				fprintf( stderr, "Dropped slow client %d\n", fd );
				slow_clients++;
				close( fd );
				FD_CLR( fd, &afds );
				exit_client( 0 );
				arrival_time[ fd ] = 0;
			}
		}

		if ( status > 0 ) {
			// Now check all the regular sockets
			for ( fd = 0; fd < nfds; fd++ ) {
				// check every socket to see if it's in the ready set
				// But don't recheck the main socket
				if ( fd != msock && FD_ISSET( fd, &rfds ) ) {
					// you can read without blocking because data is there
					// the OS has confirmed this
					arrival_time[ fd ] = 0;
					conn_handler( fd );
					FD_CLR( fd, &afds );
					// lower the max socket number if needed
					if ( nfds == fd + 1 )
						nfds--;
				}
			}
		}
	}
}