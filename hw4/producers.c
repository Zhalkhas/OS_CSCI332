#include <errno.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

char *          host, *service;
pthread_mutex_t mutex;

typedef struct connconf {
	char* randstr;
	int   strl;
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

void* producer( void* args )
{
	int   strl    = ( ( CONNCONF* ) args )->strl;
	char* randstr = ( ( CONNCONF* ) args )->randstr;
	int   csock;

	pthread_mutex_lock( &mutex );

	if ( ( csock = connectsock( host, service, "tcp" ) ) == 0 ) {
		fprintf( stderr, "Cannot connect to server.\n" );
		pthread_exit( NULL );
	}

	pthread_mutex_unlock( &mutex );

	write( csock, "PRODUCE\r\n", strlen( "PRODUCE\r\n" ) );
	int  rchar;
	char buf[ 5 ];
	if ( ( rchar = read( csock, buf, 4 ) ) < 0 ) {
		fprintf( stderr, "could not read from server\n" );
		pthread_exit( NULL );
	}
	buf[ 4 ] = '\0';
	fprintf( stderr, "read %d chars:%s\n", rchar, buf );

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
	close( csock );
}

int connectsock( char* host, char* service, char* protocol );

/*
**	Client
*/
int main( int argc, char* argv[] )
{
	int cc;
	int csock;
	int workers;

	switch ( argc ) {
		case 3:
			host    = "localhost";
			service = argv[ 1 ];
			workers = atoi( argv[ 2 ] );
			break;
		case 4:
			host    = argv[ 1 ];
			service = argv[ 2 ];
			workers = atoi( argv[ 3 ] );
			break;
		default:
			fprintf( stderr, "usage: consumer [host] port N\n" );
			exit( -1 );
			break;
	}

	pthread_t threads[ workers ];
	pthread_mutex_init( &mutex, NULL );

	for ( int i = 0; i < workers; i++ ) {
		CONNCONF* config = malloc( sizeof( CONNCONF ) );
		int       strl   = rand() % MAX_LETTERS;

		while ( strl == 0 ) {
			strl = rand() % MAX_LETTERS;
		}

		char* str = malloc( strl * sizeof( char ) );

		generate_str( str, strl );

		config->strl    = strl;
		config->randstr = str;

		pthread_create( &threads[ i ], NULL, producer, ( void* ) config );
	}
	for ( int i = 0; i < workers; i++ ) {
		pthread_join( threads[ i ], NULL );
	}
}
