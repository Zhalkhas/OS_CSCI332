#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char*argv[]){
	if(argc != 4){
		printf("Usage: myprog filename offset string\n");
		exit(-1);
	}

	char* filename = argv[1];
//	char* str = argv[3];
//	int offset = atoi(argv[2]);
		
	int fd = open(filename, O_RDWR);

	if(fd == -1){
		printf("wrong filename specified");
		exit(-1);	
	}
	printf("file descriptor : %d\n", fd);		
	close(fd);
}