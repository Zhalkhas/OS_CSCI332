#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

char *          host, *service;
pthread_mutex_t mutex;

void* consumer( void* arg )
{
	int csock, len;
	pthread_mutex_lock( &mutex );
	if ( ( csock = connectsock( host, service, "tcp" ) ) == 0 ) {
		fprintf( stderr, "Cannot connect to server\n" );
		pthread_exit( NULL );
	}
	pthread_mutex_unlock( &mutex );

	write( csock, "CONSUME\r\n", strlen( "CONSUME\r\n" ) );
	if ( read( csock, &len, sizeof( int ) ) <= 0 ) {
		fprintf( stderr, "could not get len of str\n" );
		pthread_exit( NULL );
	}
	len       = ntohl( len );
	char* str = malloc( ( len + 2 ) * sizeof( char ) );
	int   cc;
	if ( ( cc = read( csock, str, len ) ) == 0 ) {
		fprintf( stderr, "could not get str\n" );
		pthread_exit( NULL );
	}
	if ( cc != len ) {
		int ptr = 0;
		while ( cc != 0 && ptr <= len ) {
			if ( cc < 0 ) {
				fprintf( stderr, "could not get str\n" );
				pthread_exit( NULL );
			}
			cc = read( csock, str + ptr, len - ptr );
			ptr += cc;
		}
	}
	//	printf( "read %d bytes:, len:%d %d\n", cc,  strlen( str ) , len);
	str[ cc ]     = '\n';
	str[ cc + 1 ] = '\0';
	char name[ 40 ];
	sprintf( name, "%ld.txt", pthread_self() );
	int fd = open( name, O_WRONLY | O_APPEND | O_CREAT, 0640 );
	if ( write( fd, str, strlen( str ) ) < 0 ) {
		fprintf( stderr, "could not write to file\n" );
		pthread_exit( NULL );
	}
	close( fd );
	free( str );
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

	if ( argc != 4 ) {
	}

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
		pthread_create( &threads[ i ], NULL, consumer, NULL );
	}
	for ( int i = 0; i < workers; i++ ) {
		pthread_join( threads[ i ], NULL );
	}
}
