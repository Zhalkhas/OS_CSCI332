#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int insert_at( int fd, off_t offset, char *buf, int n );
int replace_at(int fd, off_t offset, char *s, int n);

int insert_at( int fd, off_t offset, char *buf, int n ){
  int fendl = lseek(fd, offset, SEEK_END) - 1;
  ssize_t wbytes, wbuf;	

  if (fendl == 0){
	replace_at(fd, offset, buf, n); 
  } else {
  	char *rbuf = malloc(fendl * sizeof(char));
	lseek(fd, offset, SEEK_SET);
  	if(read(fd, rbuf, fendl) == -1){
  		return -1;
  	}
  	wbytes = replace_at(fd, offset, buf, n);
  	wbuf = replace_at(fd, offset+n, rbuf, fendl);
	free(rbuf);
  }
  if (wbytes == -1|| wbuf == -1){
  	return -1;
  }
  
  return 0; 	
}

int replace_at(int fd, off_t offset, char *s, int n) {
  lseek(fd, offset, SEEK_SET);
  ssize_t wbytes = write(fd, s, n);
  if (wbytes == -1)
    return -1;
  else
    return offset + wbytes;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: prog file offset str\n");
    exit(-1);
  }
  
  char *filename = argv[1];
  char *str = argv[3];
  off_t offset = atoi(argv[2]);
  int strl = strlen(str);

  int fd = open(filename, O_RDWR);
  int flen = lseek(fd, 0, SEEK_END);
  if (flen < offset){
  	printf("offset out of file bounds\n");
  	exit(-1);
  }

  if(insert_at(fd, offset, str, strl) == -1){
  	printf("error ocured\n");
	exit(-1);
  }
  printf("insert success\n");
  exit(0);
}