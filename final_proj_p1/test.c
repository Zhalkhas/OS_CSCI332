#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

double poissonRandomInterarrivalDelay( double r )
{
	return ( log( ( double ) 1.0 - ( ( double ) rand() ) / ( ( double ) RAND_MAX ) ) ) / -r;
}

int main( int argc, char** argv )
{
	double ranint = atof( argv[ 1 ] );
	for ( int i = 0; i < 10; i++ ) {
        unsigned int x = poissonRandomInterarrivalDelay(ranint) * 1000;
		usleep(x);
		printf( "%d\n",x );
	}
}