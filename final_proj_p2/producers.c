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

#define SLOW_CLIENT 3

char *          host, *service;
pthread_mutex_t mutex;

typedef struct connconf {
	char* randstr;
	int   strl;
	bool  bad;
} CONNCONF;

void generate_str( char* str, int strl )
{
	int i;
	for ( i = 0; i < strl; i++ ) {
		// str[ i ] = '!' + rand() % 90;
		str[ i ] = '*';
	}
	str[ i ] = '\0';
}

/*
**      Poisson interarrival times. Adapted from various sources
**      r = desired arrival rate
*/
double poissonRandomInterarrivalDelay( double r )
{
	return ( log( ( double ) 1.0 - ( ( double ) rand() ) / ( ( double ) RAND_MAX ) ) ) / -r;
}


void* producer( void* args )
{
	int   strl    = ( ( CONNCONF* ) args )->strl;
	char* randstr = ( ( CONNCONF* ) args )->randstr;
	bool  bad     = ( ( CONNCONF* ) args )->bad;
	int   csock;

	pthread_mutex_lock( &mutex );

	if ( ( csock = connectsock( host, service, "tcp" ) ) == 0 ) {
		fprintf( stderr, "Cannot connect to server.\n" );
		pthread_exit( NULL );
	}

	pthread_mutex_unlock( &mutex );
	if ( bad )
		sleep( SLOW_CLIENT );

	write( csock, "PRODUCE\r\n", strlen( "PRODUCE\r\n" ) );
	int  rchar;
	char buf[ 7 ];
	if ( ( rchar = read( csock, buf, 4 ) ) < 0 ) {
		fprintf( stderr, "could not read from server\n" );
		pthread_exit( NULL );
	}
	buf[ 4 ] = '\0';
	// fprintf( stderr, "read %d chars:%s\n", rchar, buf );

	if ( strcmp( "GO\r\n", buf ) == 0 ) {
		fprintf( stderr, "OK:%s\n", randstr );
		strl = htonl( strl );
		write( csock, &strl, 4 );
		write( csock, randstr, strlen( randstr ) );
	} else {
		fprintf( stderr, "server closed the connection\n" );
	}

	free( ( ( CONNCONF* ) args )->randstr );
	free( ( CONNCONF* ) args );
	int cc;
	if ( ( cc = read( csock, buf, 6 ) ) == 0 ) {
		fprintf( stderr, "could not get str\n" );
		pthread_exit( NULL );
	}
	if ( cc < 5 ) {
		int ptr = 0;
		while ( cc != 0 && ptr <= 6 ) {
			if ( cc < 0 ) {
				fprintf( stderr, "could not get str\n" );
				pthread_exit( NULL );
			}
			cc = read( csock, buf + ptr, 6 - ptr );
			ptr += cc;
		}
	}
	buf[ 6 ] = '\0';

	if ( strcmp( "DONE\r\n", buf ) ) {
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
		fprintf( stderr, "usage: producer [host] port num rate bad\n" );
		exit( -1 );
	}

	bad = workers * ( bad / 100 );

	pthread_t threads[ workers ];
	pthread_mutex_init( &mutex, NULL );

	fprintf( stderr, "startup config: host - %s\r\nport - %s\r\nnumber of clients - %d"
					 "\r\nclients rate - %f\r\nbad clients - %d\r\n",
			 host, service, workers, rate, bad );

	CONNCONF* configs[ workers ];

	for ( int i = 0; i < workers; i++ ) {
		configs[ i ] = malloc( sizeof( CONNCONF ) );
		int strl     = rand() % MAX_LETTERS;

		while ( strl == 0 ) {
			strl = rand() % MAX_LETTERS;
		}

		char* str = malloc( strl * sizeof( char ) );

		generate_str( str, strl );

		configs[ i ]->strl    = strl;
		configs[ i ]->randstr = str;
		configs[ i ]->bad     = false;
	}

	for ( int i = 0; i < bad; i++ ) {
		int randind = rand() % workers;
		while ( configs[ randind ]->bad == true ) {
			randind = rand() % workers;
		}
		configs[ randind ]->bad == true;
	}


	for ( int i = 0; i < workers; i++ ) {
		usleep( 1000 * poissonRandomInterarrivalDelay( rate ) );
		pthread_create( &threads[ i ], NULL, producer, ( void* ) configs[ i ] );
	}
	for ( int i = 0; i < workers; i++ ) {
		pthread_join( threads[ i ], NULL );
	}
}
