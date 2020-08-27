#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

char *host, *service;

char* valid_vals[ 9 ] = { "CURRCLI", "CURRPROD", "CURRCONS",
						  "TOTPROD", "TOTCONS", "REJMAX",
						  "REJSLOW", "REJPROD", "REJCONS" };

int connectsock( char* host, char* service, char* protocol );

int main( int argc, char** argv )
{
	char* value;
	int   csock;
	switch ( argc ) {
		case 3:
			host    = "localhost";
			service = argv[ 1 ];
			value   = argv[ 2 ];
			break;
		case 4:
			host    = argv[ 1 ];
			service = argv[ 2 ];
			value   = argv[ 3 ];
			break;
		default:
			fprintf( stderr, "usage: status [host] port value\n" );
			exit( -1 );
			break;
	}

	if ( strlen( value ) > 8 ) {
		fprintf( stderr, "Invalid value given\n" );
		exit( -1 );
	}

	bool is_valid = false;
	for ( unsigned int i = 0; i < 9; i++ ) {
		if ( !strcmp( value, valid_vals[ i ] ) ) {
			is_valid = true;
			break;
		}
	}

	if ( !is_valid ) {
		fprintf( stderr, "Invalid value given\n" );
		fprintf( stderr, "Only following values are valid:\n" );
		for ( int i = 0; i < 3; i++ ) {
			fprintf( stderr, "%-8s %-7s %7s\n", valid_vals[ i ], valid_vals[ i + 3 ], valid_vals[ i + 6 ] );
		}
		exit( -1 );
	}
	csock = connectsock( host, service, "tcp" );
	if ( csock == 0 ) {
		fprintf( stderr, "Cannot connect to server\n" );
		exit( -1 );
	}

	char request[ 20 ];
	request[ 0 ] = '\0';

	sprintf( request, "STATUS/%s\r\n", value );
	fprintf( stderr, "Sending request %s\n", request, strlen( request ), csock );


	if ( write( csock, request, strlen( request ) ) <= 0 ) {
		fprintf( stderr, "Could not send request to server, %s\n", strerror( errno ) );
		exit( -1 );
	}

	char responce[ 12 ];
	if ( read( csock, responce, 12 ) <= 0 ) {
		fprintf( stderr, "Could not read from socket:%s\n", strerror( errno ) );
		exit( -1 );
	}
	fprintf( stderr, "STATUS/%s: %s\n", value, responce );
}