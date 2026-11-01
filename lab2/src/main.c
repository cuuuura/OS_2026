#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef struct {
    double re; 
    double im;
} Complex;

typedef struct {
    Complex *data;
    int rows;
    int cols;
} Matrix;

int parse_int(const char *str, const char *name, int min_val, int max_val, int *out) {
    char * endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        fprintf(stderr, "Ошибка: %s вне диапазона int\n", name);
        return 0;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: %s — не число ('%s')\n", name, str);
        return 0;
    }
    if (val < min_val || val > max_val) {
        fprintf(stderr, "Ошибка: %s должно быть в диапазоне [%d, %d], получено %ld\n", name, min_val, max_val, val);
        return 0;
    }
    *out = (int)val;
    return 1;
}

double get_time_sec() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

typedef struct {
    const Matrix *A;
    const Matrix *B;
    Matrix *C;
    int start_row;
    int end_row;
    int thread_id;
} ThreadArg;

Matrix* matrix_create(int rows, int cols) {
    Matrix *m = (Matrix*)malloc(sizeof(Matrix));
    if (!m) {
        fprintf(stderr, "Ошибка выделения памяти под матрицу\n");
        exit(1);
    }
    
    m->rows = rows;
    m->cols = cols;
    
    m->data = (Complex*)calloc(rows * cols, sizeof(Complex));
    if (!m->data) {
        fprintf(stderr, "Ошибка выделения памяти под данные матрицы\n");
        free(m);
        exit(1);
    }
    
    return m;
}

void matrix_free(Matrix *m) {
    if (m) {
        free(m->data);
        free(m);
    }
}

void matrix_random_data(Matrix *m) {
    for (int i = 0; i < m->rows * m->cols; i++) {
        m->data[i].re = (double)(rand() % 100) / 10.0;
        m->data[i].im = (double)(rand() % 100) / 10.0;
    }
}

int verify_results(const Matrix *C1, const Matrix *C2) {
    double epsilon = 1e-6;
    for (int i = 0; i < C1->rows * C1->cols; i++) {
        if (fabs(C1->data[i].re - C2->data[i].re) > epsilon || 
            fabs(C1->data[i].im - C2->data[i].im) > epsilon) {
            return 0; 
        }
    }
    return 1; 
}

Complex complex_mul(Complex a, Complex b) {
    Complex res;
    res.re = a.re * b.re - a.im * b.im;
    res.im = a.re * b.im + a.im * b.re;
    return res;
}

double multiply_without_threads(const Matrix *A, const Matrix *B, Matrix *C) {
    double start_time = get_time_sec();

    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            Complex sum = {0.0, 0.0};
            for (int k = 0; k < A->cols; k++) {
                Complex a = A->data[i * A->cols + k];
                Complex b = B->data[k * B->cols + j];
                Complex prod = complex_mul(a, b);
                sum.re += prod.re;
                sum.im += prod.im;
            }
            C->data[i * C->cols + j] = sum;
        }
    }

    return get_time_sec() - start_time; 
}

void* thread_func(void *arg) {
    ThreadArg *data = (ThreadArg*)arg;
    
    const Matrix *A = data->A;
    const Matrix *B = data->B;
    Matrix *C = data->C;

    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < B->cols; j++) {
            Complex sum = {0.0, 0.0};
            
            for (int k = 0; k < A->cols; k++) {
                Complex a = A->data[i * A->cols + k];
                Complex b = B->data[k * B->cols + j];
                
                Complex prod = complex_mul(a, b);
                sum.re += prod.re;
                sum.im += prod.im;
            }
            
            C->data[i * C->cols + j] = sum;
        }
    }
    
    return NULL;
}

double multiply_with_threads(const Matrix *A, const Matrix *B, Matrix *C, int num_threads) {
    if (num_threads > C->rows) num_threads = C->rows;

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg*)malloc(num_threads * sizeof(ThreadArg));

    int rows_per_thread = C->rows / num_threads;
    int remainder = C->rows % num_threads;
    int current_row = 0;

    double start_time = get_time_sec();

    for (int i = 0; i < num_threads; i++) {
        args[i].A = A;
        args[i].B = B;
        args[i].C = C;
        args[i].thread_id = i;
        
        args[i].start_row = current_row;
        int rows_to_process = rows_per_thread + (i < remainder ? 1 : 0);
        args[i].end_row = current_row + rows_to_process;
        current_row = args[i].end_row;

        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            perror("Ошибка создания потока");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    double end_time = get_time_sec();

    free(threads);
    free(args);

    return end_time - start_time;
}

void create_table_results(Matrix *A, Matrix *B, Matrix *C_1, Matrix *C_2) {
    // Список количества потоков для тестирования
    int thread_counts[] = {1, 2, 4, 8, 16, 32, 128, 1024};
    int num_counts = sizeof(thread_counts) / sizeof(thread_counts[0]);
    int runs = 3; // Количество прогонов для стабильности

    // 1. Замеряем последовательную версию (3 раза, берем лучший результат)
    double t_seq_min = 1e9;
    for (int r = 0; r < runs; r++) {
        double t = multiply_without_threads(A, B, C_1);
        if (t < t_seq_min) t_seq_min = t;
    }

    printf("Потоков\t\tВремя (мс)\tУскорение\tЭффективность\n");

    printf("Послед\t\t%.2f\t\t1.00\t\t100.0%%\n", t_seq_min * 1000.0);

    for (int i = 0; i < num_counts; i++) {
        int threads = thread_counts[i];
        double t_par_min = 1e9;
        
        for (int r = 0; r < runs; r++) {
            double t = multiply_with_threads(A, B, C_2, threads);
            if (t < t_par_min) t_par_min = t;
        }

        if (!verify_results(C_1, C_2)) {
            printf("(!) ОШИБКА: Результаты не совпадают!\n");
        }

        double speedup = t_seq_min / t_par_min;
        double efficiency = speedup / threads;

        printf("%d\t\t%.2f\t\t%.2f\t\t%.1f%%\n", threads, t_par_min * 1000.0, speedup, efficiency * 100.0);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 5) {
        int M, K, N, threads;

        if(!parse_int(argv[1], "M", 1, 20000, &M)) return 1;
        if(!parse_int(argv[2], "K", 1, 20000, &K)) return 1;
        if(!parse_int(argv[3], "N", 1, 20000, &N)) return 1;
        if(!parse_int(argv[4], "threads", 1, 100000, &threads)) return 1;

        Matrix *A = matrix_create(M, K);
        Matrix *B = matrix_create(K, N);
        Matrix *C_1 = matrix_create(M, N);
        Matrix *C_2 = matrix_create(M, N);
        
        srand(12345); 
        matrix_random_data(A);
        matrix_random_data(B);

        double t_seq = multiply_without_threads(A, B, C_1);
        double t_par = multiply_with_threads(A, B, C_2, threads);

        if (verify_results(C_1, C_2)) {
            printf("Результаты совпадают!\n");
        } else {
            printf("ОШИБКА: Результаты не совпадают!\n");
        }

        printf("Время последовательное: %.4f сек\n", t_seq);
        printf("Время параллельное: %.4f сек\n", t_par);
        
        double speedup = t_seq / t_par;
        double efficiency = speedup / threads;
        
        printf("Ускорение (S) = %.2f\n", speedup);
        printf("Эффективность (X) = %.1f%%\n", efficiency * 100.0);

        matrix_free(A); matrix_free(B); matrix_free(C_1); matrix_free(C_2);
        return 0;
    }

    if (argc == 4) {
        int M, K, N;

        if(!parse_int(argv[1], "M", 1, 20000, &M)) return 1;
        if(!parse_int(argv[2], "K", 1, 20000, &K)) return 1;
        if(!parse_int(argv[3], "N", 1, 20000, &N)) return 1;

        Matrix *A = matrix_create(M, K);
        Matrix *B = matrix_create(K, N);
        Matrix *C_1 = matrix_create(M, N);
        Matrix *C_2 = matrix_create(M, N);

        srand(12345); 
        matrix_random_data(A);
        matrix_random_data(B);

        create_table_results(A, B, C_1, C_2);
        
        matrix_free(A);
        matrix_free(B);
        matrix_free(C_1);
        matrix_free(C_2);

        return 0;
    }

    fprintf(stderr, "Использование:\n");
    fprintf(stderr, "%s [M] [K] [N] [threads] - один замер с заданным числом потоков\n", argv[0]);
    fprintf(stderr, "%s [M] [K] [N] - автоматическая таблица\n", argv[0]);
    return 1;

    return 0;
}