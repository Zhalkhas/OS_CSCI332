# HW 1

This is extension of file I/O labs

# Requirements
* Instead of the replace_at function, you should write the function:
```C
insert_at( int fd, off_t offset, char *buf, int n )
```
This function should insert the n chars from the buf at the offset in the file.  If bytes are already there, the new bytes should be inserted in between them.  If there are no bytes already there, then the new bytes just should be written.  The destination file’s size should be num bytes larger after the program runs sucessfully.
* Program should give error messages about any problems with the arguments or the files or the I/O.