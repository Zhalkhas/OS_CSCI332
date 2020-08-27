#include <errno.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum { PRODUCER = 1,
			   CONSUMER = 2 } client_type;

ITEM**          buf;
int             bufsize, count = 0;
int             clients = 0, prods = 0, cons = 0;
pthread_mutex_t buf_mutex, client_mutex;
sem_t           full, empty;

ITEM* makeItem( unsigned int len )
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
	// printf( "status: cons - %d prods - %d cli - %d\n", cons, prods, clients );

	clients--;
	switch ( type ) {
		case PRODUCER:
			prods--;
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
		exit_client( PRODUCER );
		pthread_exit( NULL );
	}

	int ilen;

	if ( read( ssock, &ilen, sizeof( ilen ) ) < 0 ) {
		fprintf( stderr, "could not read item len\n" );
		exit_client( PRODUCER );
		pthread_exit( NULL );
	}


	ilen = ntohl( ilen );

	ITEM* i    = makeItem( ilen );
	i->prod_sd = ssock;

	pthread_mutex_lock( &buf_mutex );

	buf[ count ] = i;
	count++;

	pthread_mutex_unlock( &buf_mutex );

	sem_post( &full );
	exit_client( PRODUCER );
}

void* consumer( void* arg )
{
	int ssock = *( int* ) arg;
	sem_wait( &full );
	pthread_mutex_lock( &buf_mutex );

	ITEM* i          = buf[ count - 1 ];
	buf[ count - 1 ] = NULL;
	count--;

	pthread_mutex_unlock( &buf_mutex );
	sem_post( &empty );

	int size = htonl( i->size );

	if ( write( ssock, &size, 4 ) < 0 ) {
		fprintf( stderr, "could not send item len\n" );
		exit_client( CONSUMER );
	}
	int   cc, ptr = 0;
	char* cbuf = malloc( i->size * sizeof( char ) );
	cc         = read( i->prod_sd, cbuf, i->size );
	if ( cc != i->size ) {
		while ( cc != 0 && ptr <= i->size ) {
			if ( cc < 0 ) {
				fprintf( stderr, "could not get str\n" );
				exit_client( CONSUMER );
			}
			cc = read( i->prod_sd, cbuf + ptr, i->size - ptr );
			ptr += cc;
		}
	}
	cc  = write( ssock, cbuf, i->size );
	ptr = 0;
	if ( cc != i->size ) {
		while ( cc != 0 && ptr <= i->size ) {
			if ( cc < 0 ) {
				fprintf( stderr, "could not get str\n" );
				exit_client( CONSUMER );
			}
			cc = write( ssock, cbuf + ptr, i->size - ptr );
			ptr += cc;
		}
	}
	write( i->prod_sd, "DONE\r\n", 7 );
	exit_client( CONSUMER );
}

bool is_free( client_type type )
{
	switch ( type ) {
		case PRODUCER:
			return prods < MAX_PROD;
		case CONSUMER:
			return cons < MAX_CON;
	}
}

void* conn_handler( void* arg )
{
	pthread_mutex_lock( &client_mutex );
	clients++;
	pthread_mutex_unlock( &client_mutex );
	int  ssock = *( int* ) arg;
	char charbuf[ 10 ];

	if ( read( ssock, charbuf, 9 ) <= 0 ) {
		fprintf( stderr, "could not read from socket %s\n", strerror( errno ) );
	}
	charbuf[ 9 ] = '\0';
	if ( strcmp( "PRODUCE\r\n", charbuf ) == 0 ) {
		pthread_mutex_lock( &client_mutex );
		bool free = is_free( PRODUCER );
		prods += free;
		pthread_mutex_unlock( &client_mutex );
		free ? producer( ( void* ) &ssock ) : exit_client( 0 );
	} else if ( strcmp( "CONSUME\r\n", charbuf ) == 0 ) {
		pthread_mutex_lock( &client_mutex );
		bool free = is_free( CONSUMER );
		cons += free;
		pthread_mutex_unlock( &client_mutex );
		free ? consumer( ( void* ) &ssock ) : exit_client( 0 );
	} else {
		exit_client( 0 );
		close( ssock );
		pthread_exit( NULL );
	}
}

int main( int argc, char** argv )
{
	char*              service;
	int                msock;
	int                rport = 0;
	socklen_t          alen;
	struct sockaddr_in fsin;
	int                ssock;
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
			bufsize = atoi( argv[ 1 ] );
			service = argv[ 2 ];
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

	for ( ;; ) {

		memcpy( ( char* ) &rfds, ( char* ) &afds, sizeof( rfds ) );
		if ( select( nfds, &rfds, ( fd_set* ) 0, ( fd_set* ) 0,
					 ( struct timeval* ) 0 )
			 < 0 ) {
			fprintf( stderr, "server select: %s\n", strerror( errno ) );
			exit( -1 );
		}

		if ( FD_ISSET( msock, &rfds ) ) {
			int ssock;

			// we can call accept with no fear of blocking
			alen  = sizeof( fsin );
			ssock = accept( msock, ( struct sockaddr* ) &fsin, &alen );
			if ( ssock < 0 ) {
				fprintf( stderr, "accept: %s\n", strerror( errno ) );
				exit( -1 );
			}

			// If a new client arrives, we must add it to our afds set
			FD_SET( ssock, &afds );

			// and increase the maximum, if necessary
			if ( ssock + 1 > nfds )
				nfds = ssock + 1;
		}

		// Now check all the regular sockets
		for ( fd = 0; fd < nfds; fd++ ) {
			// check every socket to see if it's in the ready set
			// But don't recheck the main socket
			if ( fd != msock && FD_ISSET( fd, &rfds ) ) {
				// you can read without blocking because data is there
				// the OS has confirmed this
				pthread_mutex_lock( &client_mutex );
				bool free = clients < MAX_CLIENTS;
				pthread_mutex_unlock( &client_mutex );
				int* sockfd = malloc( sizeof( int ) );
				memcpy( sockfd, &fd, sizeof( fd ) );
				free ? pthread_create( &thread, NULL, conn_handler, ( void* ) sockfd ) : close( *sockfd );
				FD_CLR( fd, &afds );
				// lower the max socket number if needed
				if ( nfds == fd + 1 )
					nfds--;
			}
		}
	}
}