# Lab 4

This is implementation of matrix multiplication in C using multithreading

## Requirements

### Part 1

* Hard-code two operand matrices and one result matrix globally.
* Write a sequential solution to this problem.
* Time the computation: you may use the function call gettimeofday, which can give you microseconds, to get the time before and after your computation is finished.

### Part 2
* Divide the matrix multiplication task, assigning one thread per row of the first matrix.
* Output the result matrix and check your solution.
* Does this division of work require any synchronization mechanisms?  Is it faster than sequential?