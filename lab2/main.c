#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

size_t str_len (const char *str){
    return (*str) ? str_len(++str) + 1 : 0;
}

int replace_at( int fd, off_t offset, char *s, int n ){
	lseek(fd, offset, SEEK_SET);
	ssize_t wbytes = write(fd, s, n);
	if(wbytes == -1) return -1;
	else return offset + wbytes;
}

int main(int argc, char*argv[]){
	if(argc != 4){
		write(1, "Usage: replace filename offset string\n", 38);
		exit(-1);
	}

	char* filename = argv[1];
	char* str = argv[3];
	unsigned int strl = strlen(str);
	off_t offset = atoi(argv[2]);
		
	int fd = open(filename, O_RDWR);

	if(fd == -1){
		write(1, "wrong filename specified\n", 25);
		exit(-1);	
	}

	int flen = lseek(fd, 0, SEEK_END) - 1;
	if(strl + offset > flen){
		write(1, "offset is out of file boundaries\n", 33);
		exit( -1);
	}
	if(replace_at(fd, offset, str, strl) == -1){
		write(1, "replace failed\n", 15);
		exit(-1);
	} 
	write(1, "replace successful\n", 19);
	close(fd);
}