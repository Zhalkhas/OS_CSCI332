# LAB 3

This lab is extension of the lab 2

## Requirements

* Program accept 4 arguments: a source file name, a destination file name, a file offset, and a number of characters.  
```bash 
prog sourcefile destfile offset num
```

* Program reads _num_ bytes from the _offset_ in _sourcefile_ and writes those _num_ bytes at the same _offset_ in _destfile_

* You must use the system calls listed above, not stdio functions.  You may use printf for printing messages and string.h functions. 
* Do not make any assumptions about the contents of the files or their sizes.
* You must assure that the source file exists and is large enough to be able to take num bytes from the requested offset.  
* The destination file does not have to exist and should be created if it doesn’t exist.  Thus the file size also doesn’t matter – just write the bytes in the requested location.

# Note 
If you need to examine a file that begins with nulls, you may have to use a program called od, for octal dump: 
```bash 
od -c filename
```