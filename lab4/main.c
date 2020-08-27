#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define ROWM1 2
#define COLM1 3
#define COLM2 2

int m1[2][3] = {{1, 2, 3},
                {4, 5, 6}};
int m2[3][2] = {{1, 2},
                {3, 4},
                {5, 6}};
int result_t[2][2] = {{0, 0},
                      {0, 0}};

int result[2][2] = {{0, 0},
                    {0, 0}};

void *add_row(void *index) {
    int i = *(int *) index;
    printf("index:%d\n", i);
    for (int j = 0; j < COLM2; j++) {
        for (int k = 0; k < COLM1; k++) {
            result_t[i][j] += m1[i][k] * m2[k][j];
        }
    }
    free(index);
    pthread_exit(NULL);
}

void add_matrix_t() {
    pthread_t threads[ROWM1];
    for (int i = 0; i < ROWM1; i++) {
        int *n = malloc(sizeof(int));
        *n = i;
        if (pthread_create(&threads[i], NULL, add_row, (void *) n) != 0) {
            printf("could not create thread %d\n", i);
        }
    }
    for (int i = 0; i < ROWM1; i++) {
        pthread_join(threads[i], NULL);
    }
}

void add_matrix() {
    for (int i = 0; i < ROWM1; i++) {
        for (int j = 0; j < COLM2; j++) {
            for (int k = 0; k < COLM1; k++) {
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

void print_matrix(int row, int col, int mat[row][col]) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

int main() {
    unsigned long start = get_time();
    add_matrix_t();
    unsigned long end = get_time();
    print_matrix(ROWM1, COLM2, result_t);
    printf("concurrent elapsed time: %ld us\n", end - start);

    start = get_time();
    add_matrix();
    end = get_time();
    print_matrix(ROWM1, COLM2, result);
    printf("sequential elapsed time: %ld us\n", end - start);

}