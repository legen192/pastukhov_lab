#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <omp.h>

#define N 1000

void sequential_matrix_multiply(const std::vector<std::vector<double>>& A, 
                               const std::vector<std::vector<double>>& B, 
                               std::vector<std::vector<double>>& C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void parallel_matrix_multiply(const std::vector<std::vector<double>>& A, 
                             const std::vector<std::vector<double>>& B, 
                             std::vector<std::vector<double>>& C, int n) {
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void generate_random_matrix(std::vector<std::vector<double>>& matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = ((double)rand() / RAND_MAX) * 10.0;  // Випадкові числа від 0 до 10
        }
    }
}

void write_matrix_to_file(const std::vector<std::vector<double>>& matrix, int n, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Помилка відкриття файлу " << filename << " для запису" << std::endl;
        exit(1);
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            file << matrix[i][j] << " ";
        }
        file << std::endl;
    }
    
    file.close();
    std::cout << "Матриця записана у файл " << filename << std::endl;
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    std::cout << "Створення матриць розміром " << N << "x" << N << "..." << std::endl;
    std::vector<std::vector<double>> A(N, std::vector<double>(N));
    std::vector<std::vector<double>> B(N, std::vector<double>(N));
    std::vector<std::vector<double>> C_seq(N, std::vector<double>(N));
    std::vector<std::vector<double>> C_par(N, std::vector<double>(N));
    
    std::cout << "Генерація випадкових матриць..." << std::endl;
    generate_random_matrix(A, N);
    generate_random_matrix(B, N);
    
    write_matrix_to_file(A, N, "matrix_A.txt");
    write_matrix_to_file(B, N, "matrix_B.txt");
    
    std::cout << "Виконання послідовного множення матриць..." << std::endl;
    double seq_start = omp_get_wtime();
    
    sequential_matrix_multiply(A, B, C_seq, N);
    
    double seq_end = omp_get_wtime();
    double seq_time = seq_end - seq_start;
    
    std::cout << "Виконання паралельного множення матриць..." << std::endl;
    
    int thread_counts[] = {2, 4, 8, 16};
    int num_tests = sizeof(thread_counts) / sizeof(int);
    double par_times[num_tests];
    
    for (int t = 0; t < num_tests; t++) {
        int num_threads = thread_counts[t];
        omp_set_num_threads(num_threads);
        
        std::cout << "- Використання " << num_threads << " потоків..." << std::endl;
        double par_start = omp_get_wtime();
        
        parallel_matrix_multiply(A, B, C_par, N);
        
        double par_end = omp_get_wtime();
        par_times[t] = par_end - par_start;
    }
    
    bool is_correct = true;
    for (int i = 0; i < N && is_correct; i++) {
        for (int j = 0; j < N && is_correct; j++) {
            if (fabs(C_seq[i][j] - C_par[i][j]) > 1e-10) {
                is_correct = false;
                std::cout << "Помилка: відмінності в результатах (" << i << ", " << j 
                         << "): " << C_seq[i][j] << " != " << C_par[i][j] << std::endl;
                break;
            }
        }
    }
    
    if (is_correct) {
        std::cout << "Перевірка: результати послідовного та паралельного множення співпадають." << std::endl;
    }
    
    write_matrix_to_file(C_seq, N, "result_sequential.txt");
    write_matrix_to_file(C_par, N, "result_parallel.txt");
    
    std::cout << "\nРезультати порівняння" << std::endl;
    std::cout << "Розмірність матриць: " << N << " x " << N << std::endl;
    std::cout << "Послідовне множення: " << seq_time << " секунд" << std::endl;
    
    std::cout << "\nПаралельне множення за кількістю потоків:" << std::endl;
    for (int t = 0; t < num_tests; t++) {
        std::cout << "- " << thread_counts[t] << " потоків: " << par_times[t] << " секунд" << std::endl;
    }
    
    return 0;
}
