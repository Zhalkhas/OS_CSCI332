#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

char *          host, *service;
pthread_mutex_t mutex;

/*
**      Poisson interarrival times. Adapted from various sources
**      r = desired arrival rate
*/
double poissonRandomInterarrivalDelay( double r )
{
	return ( log( ( double ) 1.0 - ( ( double ) rand() ) / ( ( double ) RAND_MAX ) ) ) / -r;
}

void generate_str( char str[], int strlen )
{
	for ( int i = 0; i < strlen; i++ ) {
		str[ i ] = '!' + rand() % 90;
	}
}

void* producer( void* args )
{
	bool bad = *( bool* ) args;
	int  csock;

	pthread_mutex_lock( &mutex );

	if ( ( csock = connectsock( host, service, "tcp" ) ) == 0 ) {
		fprintf( stderr, "Cannot connect to server.\n" );
		pthread_exit( NULL );
	}

	pthread_mutex_unlock( &mutex );
	if ( bad ) {
		sleep( SLOW_CLIENT );
	}
	if ( write( csock, "PRODUCE\r\n", strlen( "PRODUCE\r\n" ) ) < 0 ) {
		fprintf( stderr, "Server dropped the connection\n" );
		close( csock );
		pthread_exit( NULL );
	}
	int  rchar;
	char buf[ 7 ];
	if ( ( rchar = read( csock, buf, 4 ) ) < 0 ) {
		fprintf( stderr, "could not read from server\n" );
		close( csock );
		pthread_exit( NULL );
	}
	buf[ 4 ] = '\0';

	uint32_t strl = rand() % MAX_LETTERS;
	while ( strl == 0 ) {
		strl = rand() % MAX_LETTERS;
	}

	if ( strcmp( "GO\r\n", buf ) == 0 ) {
		uint32_t htonstrl = htonl( strl );
		write( csock, &htonstrl, 4 );
		if ( ( rchar = read( csock, buf, 4 ) ) < 0 ) {
			fprintf( stderr, "could not read from server\n" );
			close( csock );
			pthread_exit( NULL );
		}
		buf[ 4 ] = '\0';
		if ( strcmp( "GO\r\n", buf ) == 0 ) {
			char* charbuf = malloc( BUFSIZE * sizeof( char ) );
			generate_str( charbuf, BUFSIZE );
			int chunk_count = strl / BUFSIZE;
			for ( uint32_t i = 0; i < chunk_count; i++ ) {
				write( csock, charbuf, BUFSIZE );
			}
			int left = strl % BUFSIZE;
			if ( left != 0 ) {
				write( csock, charbuf, left );
			}
		}
	} else {
		fprintf( stderr, "server closed the connection\n" );
		close( csock );
		pthread_exit( 0 );
	}

	int cc;
	if ( ( cc = read( csock, buf, 6 ) ) < 0 ) {
		fprintf( stderr, "could not get str\n" );
		close( csock );
		pthread_exit( NULL );
	}
	if ( cc < 5 ) {
		int ptr = 0;
		while ( cc != 0 && ptr <= 6 ) {
			if ( cc < 0 ) {
				fprintf( stderr, "could not get str\n" );
				close( csock );
				pthread_exit( NULL );
			}
			cc = read( csock, buf + ptr, 6 - ptr );
			ptr += cc;
		}
	}
	buf[ 6 ] = '\0';

	if ( strcmp( "DONE\r\n", buf ) ) {
		fprintf( stderr, "DONE\n" );
		close( csock );
	}
}

int connectsock( char* host, char* service, char* protocol );

/*
**	Client
*/
int main( int argc, char* argv[] )
{
	int   cc;
	int   csock;
	int   workers;
	float rate;
	int   bad;

	switch ( argc ) {
		case 5:
			host    = "localhost";
			service = argv[ 1 ];
			workers = atoi( argv[ 2 ] );
			rate    = atof( argv[ 3 ] );
			bad     = atoi( argv[ 4 ] );
			break;
		case 6:
			host    = argv[ 1 ];
			service = argv[ 2 ];
			workers = atoi( argv[ 3 ] );
			rate    = atof( argv[ 3 ] );
			bad     = atoi( argv[ 4 ] );
			break;
		default:
			fprintf( stderr, "usage: producer [host] port num rate bad\n" );
			exit( -1 );
			break;
	}

	if ( bad < 0 || bad > 100 || service < 0 || workers < 0 || workers > 2000 || rate < 0 ) {
		fprintf( stderr, "Wrong argument value provided\n" );
		exit( -1 );
	}

	bad = ( float ) ( workers * ( bad / 100.0 ) );

	pthread_t threads[ workers ];
	pthread_mutex_init( &mutex, NULL );

	// fprintf( stderr, "startup config: host - %s\r\nport - %s\r\nnumber of clients - %d"
	// 				 "\r\nclients rate - %f\r\nbad clients - %d\r\n",
	// 		 host, service, workers, rate, bad );

	bool bad_producers[ workers ];
	memset( bad_producers, false, workers );

	for ( int i = 0; i < bad; i++ ) {
		int index = rand() % workers;
		while ( bad_producers[ index ] == true ) {
			index = rand() % workers;
		}
		bad_producers[ index ] = true;
	}

	for ( int i = 0; i < workers; i++ ) {
		usleep( 1000 * poissonRandomInterarrivalDelay( rate ) );
		pthread_create( &threads[ i ], NULL, producer, ( void* ) &bad_producers[ i ] );
	}
	for ( int i = 0; i < workers; i++ ) {
		pthread_join( threads[ i ], NULL );
	}
}
