# HW 3

This mini homework is continuation of matrix multiplication lab

## Requirements

* The arguments to the program (let’s call it mm) must be:
```bash
mm rows cols val1 val2
```
* These arguments mean that you should generate a matrix m1:
Size: rows X cols
Every cell contains the value val1
And a matrix m2:
Size: cols X rows
Every cell contains the value val2
* The maximum number of rows or cols your program should handle is 1024.
* Your program should multiply the two matrices (m1 X m2) using a sequential method and print out the elapsed time to stderr.
* Your program should then multiply the two matrices using one thread per row (in m1) and print out the elapsed time to stderr.
* Your program should print out the result matrix of only the parallel computation to stdout using exactly the code below (which prints to stdout with printf).   Of course, you will use your own variable names, but the output spacing must be exactly as written here.
```C
for ( i = 0; i < resrows; i++ )
{
     for ( j = 0; j < rescols; j++ )
          printf( " %d ", result_T[i][j] );
     printf( "\n" );
}
```
* Your program should print absolutely nothing else to stdout; any debugging statements you have for yourself should go to stderr.
* You may use global variables freely.