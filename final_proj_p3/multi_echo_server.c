#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define QLEN    5
#define BUFSIZE 4096

int passivesock( char* service, char* protocol, int qlen, int* rport );

/*
**	The server ... uses multiplexng to switch between clients
**	Each client gets one echo per turn, 
**	but can have as many echoes as it wants until it disconnects
*/
int main( int argc, char* argv[] )
{
	char               buf[ BUFSIZE ];
	char*              service;
	struct sockaddr_in fsin;
	int                msock;
	int                ssock;
	fd_set             rfds;
	fd_set             afds;
	int                alen;
	int                fd;
	int                nfds;
	int                rport = 0;
	int                cc;

	// Same arguments as usual
	switch ( argc ) {
		case 1:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			break;
		case 2:
			// User provides a port? then use it
			service = argv[ 1 ];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit( -1 );
	}

	// Create the main socket as usual
	// This is the socket for accepting new clients
	msock = passivesock( service, "tcp", QLEN, &rport );
	if ( rport ) {
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );
		fflush( stdout );
	}

	// Now we begin the set up for using select

	// nfds is the largest monitored socket + 1
	// there is only one socket, msock, so nfds is msock +1
	// Set the max file descriptor being monitored
	nfds = msock + 1;

	// the variable afds is the fd_set of sockets that we want monitored for
	// a read activity
	// We initialize it to empty
	FD_ZERO( &afds );

	// Then we put msock in the set
	FD_SET( msock, &afds );

	// Now start the loop
	for ( ;; ) {
		// Since select overwrites the fd_set that we send it,
		// we copy afds into another variable, rfds
		// Reset the file descriptors you are interested in
		memcpy( ( char* ) &rfds, ( char* ) &afds, sizeof( rfds ) );

		// Only waiting for sockets who are ready to read
		//  - this includes new clients arriving
		//  - this also includes the client closed the socket event
		// We pass null for the write event and exception event fd_sets
		// we pass null for the timer, because we don't want to wake early
		if ( select( nfds, &rfds, ( fd_set* ) 0, ( fd_set* ) 0,
					 ( struct timeval* ) 0 )
			 < 0 ) {
			fprintf( stderr, "server select: %s\n", strerror( errno ) );
			exit( -1 );
		}

		// Since we've reached here it means one or more of our sockets has something
		// that is ready to read
		// So now we have to check all the sockets in the rfds set which select uses
		// to return a;; the sockets that are ready

		// If the main socket is ready - it means a new client has arrived
		// It must be checked separately, because the action is different
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
				if ( ( cc = read( fd, buf, BUFSIZE ) ) <= 0 ) {
					printf( "The client has gone.\n" );
					( void ) close( fd );

					// If the client has closed the connection, we need to
					// stop monitoring the socket (remove from afds)

				} else {
					// Otherwise send the echo to the client
					write( fd, buf, cc );
				}
				FD_CLR( fd, &afds );
				// lower the max socket number if needed
				if ( nfds == fd + 1 )
					nfds--;
			}
		}
	}
}
