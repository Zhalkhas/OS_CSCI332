#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int replace_at(int fd, off_t offset, char *s, int n) {
  lseek(fd, offset, SEEK_SET);
  ssize_t wbytes = write(fd, s, n);
  if (wbytes == -1)
    return -1;
  else
    return offset + wbytes;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    printf("Usage: prog source dest offset num\n");
    exit(-1);
  }
  
  char *src = argv[1];
  char *dest = argv[2];
  off_t offset = atoi(argv[3]);
  int num = atoi(argv[4]);

  printf("Starting with source:%s destination:%s offset:%d number of chars:%d\n", src,
         dest, offset, num);

  int fd_s = open(src, O_RDWR);

  if (fd_s == -1) {
    printf("wrong filename specified\n");
    exit(-1);
  }

  int flen = lseek(fd_s, 0, SEEK_END) - 1;
  printf("File size is %d\n", flen);
  if (num + offset > flen) {
    printf("offset is out of file boundaries\n");
    exit(-1);
  }

  if (lseek(fd_s, offset, SEEK_SET) == -1) {
    printf("could not move to offset\n");
    exit(-1);
  }

  char *buf = (char *)calloc(num, sizeof(char));

  if (buf == NULL) {
    printf("could not allocate buffer\n");
  }

  int r = read(fd_s, buf, num);
  if (r == -1) {
    printf("error while reading\n");
  }
  
  if (r != num) {
    printf("Bytes read:%d less than expected:%d\n", r, num);
    printf("Buffer:%s", buf);
  }

  close(fd_s);

  int fd_d = open(dest, O_RDWR | O_CREAT, 0640);
  if (fd_d == -1) {
    printf("could not open new file\n");
    exit(-1);
  }
  if (replace_at(fd_d, offset, buf, num) == -1) {
    printf("replace failed\n");
    exit(-1);
  }
  printf("Replace successful\n");

  close(fd_d);
}