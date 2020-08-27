#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char*argv[]){
	if(argc != 4){
		write(1, "Usage: replace filename offset string\n", 38);
		exit(-1);
	}

	char* filename = argv[1];
	char* str = argv[3];
	unsigned int strl = strlen(str);
	off_t offset = atoi(argv[2]);
		
	int fd = open(filename, O_RDWR|O_CREAT, 0640);
	lseek(fd, offset, SEEK_SET);
	write(fd, str, strl);
}