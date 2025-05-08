#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

typedef struct {
    int thread_id;
    int thread_count;
    int iterations;
    double partial_sum;
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* calculate_partial_sum(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    double local_sum = 0.0;
    
    for (int i = data->thread_id + 1; i <= data->iterations; i += data->thread_count) {
        local_sum += 1.0 / ((double)i * i);
    }
    
    data->partial_sum = local_sum;
    
    pthread_exit(NULL);
}

double measure_time(int thread_count, int iterations) {
    pthread_t threads[thread_count];
    ThreadData thread_data[thread_count];
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < thread_count; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].thread_count = thread_count;
        thread_data[i].iterations = iterations;
        thread_data[i].partial_sum = 0.0;
        
        pthread_create(&threads[i], NULL, calculate_partial_sum, (void*)&thread_data[i]);
    }
    
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double total_sum = 0.0;
    for (int i = 0; i < thread_count; i++) {
        total_sum += thread_data[i].partial_sum;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + 
                    (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("Потоків: %d, Сума: %.15f, Час: %.3f мс\n", 
           thread_count, total_sum, time_ms);
    
    return time_ms;
}

int main(int argc, char* argv[]) {
    int max_threads = 16;
    int iterations = 1000000;
    
    if (argc > 1) {
        max_threads = atoi(argv[1]);
    }
    if (argc > 2) {
        iterations = atoi(argv[2]);
    }
    
    printf("Обчислення суми ряду 1/n² від n=1 до n=%d\n", iterations);
    printf("Теоретичне значення при n->∞: π²/6 ≈ 1.644934066848226\n\n");
    
    FILE* data_file = fopen("timing_data.txt", "w");
    if (!data_file) {
        perror("Помилка при створенні файлу");
        return 1;
    }
    
    fprintf(data_file, "# Потоків\tЧас (мс)\n");
    
    for (int thread_count = 1; thread_count <= max_threads; thread_count++) {
        double time_ms = measure_time(thread_count, iterations);
        fprintf(data_file, "%d\t%.3f\n", thread_count, time_ms);
    }
    
    fclose(data_file);
    
    FILE* gnuplot_script = fopen("plot_script.gp", "w");
    if (gnuplot_script) {
        fprintf(gnuplot_script, "set terminal png size 800,600\n");
        fprintf(gnuplot_script, "set output 'timing_graph.png'\n");
        fprintf(gnuplot_script, "set title 'Залежність часу обчислення від кількості потоків'\n");
        fprintf(gnuplot_script, "set xlabel 'Кількість потоків'\n");
        fprintf(gnuplot_script, "set ylabel 'Час виконання (мс)'\n");
        fprintf(gnuplot_script, "set grid\n");
        fprintf(gnuplot_script, "plot 'timing_data.txt' using 1:2 with linespoints lw 2 pt 7 title 'Час виконання'\n");
        fclose(gnuplot_script);
        
        printf("Створено скрипт plot_script.gp для gnuplot\n");
    }
    
    return 0;
}
