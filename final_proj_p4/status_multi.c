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

bool check_value( char* value )
{
	if ( strlen( value ) > 8 ) {
		return false;
	}
	bool is_valid = false;
	for ( unsigned int i = 0; i < 9; i++ ) {
		if ( !strcmp( value, valid_vals[ i ] ) ) {
			is_valid = true;
			break;
		}
	}

	return is_valid;
}

void send_to_server( char* value )
{
	char responce[ 7 ] = { 0 };
	char request[ 18 ] = { 0 };
	sprintf( request, "STATUS/%s\r\n", value );
	printf( "Sending %s to %s:%s\n", value, host, service );
	int csock = connectsock( host, service, "tcp" );
	if ( csock == 0 ) {
		fprintf( stderr, "Cannot connect to server %s\n", strerror( errno ) );
		exit( -1 );
	}
	fprintf(stderr, "Connected to server\n");
	if ( write( csock, request, strlen( request ) ) <= 0 ) {
		printf( "Could not send to server\n" );
		return;
	}
		fprintf(stderr, "Sent to server\n");

	if ( read( csock, responce, 12 ) <= 0 ) {
		printf( "The server has gone\n" );
		return;
	}
	printf( "STATUS/%s: %s", value, responce );
	close( csock );
}

int main( int argc, char** argv )
{
	char value[ 17 ];

	switch ( argc ) {
		case 2:
			host    = "localhost";
			service = argv[ 1 ];
			break;
		case 3:
			host    = argv[ 1 ];
			service = argv[ 2 ];
			break;
		default:
			fprintf( stderr, "usage: status [host] port\n" );
			exit( -1 );
			break;
	}

	printf( "Enter the status value you want to get or q to quit\n" );
	printf( "Or write one of corresponding commands\n" );

	for ( int i = 0; i < 3; i++ ) {
		printf( "%d:%-8s\t%d:%-7s\t%d:%7s\n", i + 1, valid_vals[ i ], i + 4, valid_vals[ i + 3 ], i + 7, valid_vals[ i + 6 ] );
	}

	while ( fgets( value, 17, stdin ) != NULL ) {

		value[ strlen( value ) - 1 ] = '\0';

		if ( strlen( value ) == 0 ) {
			continue;
		}

		if ( value[ 0 ] == 'Q' || value[ 0 ] == 'q' ) {

			printf( "Exitting client\n" );
			return 0;

		} else if ( strlen( value ) == 1 ) {

			char selected_val = value[ 0 ] - '0';

			if ( selected_val < 10 && selected_val > 0 ) {
				send_to_server( valid_vals[ selected_val - 1 ] );
			} else {
				printf( "Invalid input\n" );
				continue;
			}

		} else {

			if ( !check_value( value ) ) {
				printf( "Invalid input\n" );
				continue;
			}

			send_to_server( value );
		}
	}
}