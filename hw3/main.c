#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_COL_ROW 1024
int cols, rows, **m1, **m2, **result, **result_t;

void *add_row(void *index) {
    int i = *(int *) index;
    for (int j = 0; j < rows; j++) {
        for (int k = 0; k < cols; k++) {
            result_t[i][j] += m1[i][k] * m2[k][j];
        }
    }
    free(index);
    pthread_exit(NULL);
}

void add_matrix_t() {
    pthread_t threads[rows];
    for (int i = 0; i < rows; i++) {
        int *n = malloc(sizeof(int));
        *n = i;
        if (pthread_create(&threads[i], NULL, add_row, (void *) n) != 0) {
            fprintf(stderr, "could not create thread %d\n", i);
        }
    }
    for (int i = 0; i < rows; i++) {
        pthread_join(threads[i], NULL);
    }
}

void add_matrix() {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < rows; j++) {
            for (int k = 0; k < cols; k++) {
                result[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }
}

unsigned long get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec + (tv.tv_sec * 1000000);
}

void print_matrix(int row, int col, int **mat) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++)
            printf(" %d ", mat[i][j]);
        printf("\n");
    }
}

int **create_matrix(int row, int col, int val) {
    int **m = malloc(row * sizeof(int *));
    for (int i = 0; i < row; i++) {
        m[i] = malloc(col * sizeof(int));
    }
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; ++j) {
            m[i][j] = val;
        }
    }
    return m;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: mm rows cols val1 val2\n");
        exit(-1);
    }
    rows = atoi(argv[1]);
    cols = atoi(argv[2]);

    if (rows >= MAX_COL_ROW || rows <= 0 || cols >= MAX_COL_ROW || cols <= 0) {
        fprintf(stderr, "err: wrong args\n");
        exit(-1);
    }

    int val1 = atoi(argv[3]);
    int val2 = atoi(argv[4]);

    m1 = create_matrix(rows, cols, val1);
    m2 = create_matrix(cols, rows, val2);
    result_t = create_matrix(rows, rows, 0);
    result = create_matrix(rows, rows, 0);

    unsigned long start = get_time();
    add_matrix_t();
    unsigned long end = get_time();
    fprintf(stderr, "concurrent elapsed time: %ld us\n", end - start);
    print_matrix(rows, rows, result_t);
    start = get_time();
    add_matrix();
    end = get_time();
    fprintf(stderr, "sequential elapsed time: %ld us\n", end - start);

}