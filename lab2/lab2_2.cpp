#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <string>

class Matrix {
private:
    size_t rows;
    size_t cols;
    std::vector<std::vector<double>> data;
    
public:
    Matrix(size_t r = 0, size_t c = 0) : rows(r), cols(c) {
        data.resize(rows, std::vector<double>(cols, 0.0));
    }
    
    std::vector<double>& operator[](size_t i) {
        return data[i];
    }
    
    const std::vector<double>& operator[](size_t i) const {
        return data[i];
    }
    
    size_t getRows() const { return rows; }
    size_t getCols() const { return cols; }
    
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename.c_str());
        if (!file.is_open()) {
            std::cerr << "Помилка відкриття файлу: " << filename << std::endl;
            return false;
        }
        
        file >> rows >> cols;
        
        data.resize(rows, std::vector<double>(cols, 0.0));
        
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                file >> data[i][j];
            }
        }
        
        file.close();
        return true;
    }
    
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename.c_str());
        if (!file.is_open()) {
            std::cerr << "Помилка відкриття файлу для запису: " << filename << std::endl;
            return false;
        }
        
        file << rows << " " << cols << std::endl;
        
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                file << data[i][j] << " ";
            }
            file << std::endl;
        }
        
        file.close();
        return true;
    }
    
    void print() const {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                std::cout << std::setw(8) << std::fixed << std::setprecision(2) << data[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
};

std::mutex cout_mutex;

void loadMatrixFromFile(Matrix& matrix, const std::string& filename) {
    if (matrix.loadFromFile(filename)) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Матриця завантажена з файлу: " << filename << std::endl;
        std::cout << "Розмір: " << matrix.getRows() << "x" << matrix.getCols() << std::endl;
    } else {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Помилка завантаження матриці з файлу: " << filename << std::endl;
    }
}

void multiplyMatricesPart(const Matrix& A, const Matrix& B, Matrix& C, size_t startRow, size_t endRow) {
    for (size_t i = startRow; i < endRow; ++i) {
        for (size_t j = 0; j < B.getCols(); ++j) {
            C[i][j] = 0.0;
            for (size_t k = 0; k < A.getCols(); ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

Matrix multiplyMatrices(const Matrix& A, const Matrix& B, size_t numThreads) {
    if (A.getCols() != B.getRows()) {
        std::cerr << "Помилка: Неможливо помножити матриці. Несумісні розміри." << std::endl;
        return Matrix(0, 0);
    }
    
    Matrix C(A.getRows(), B.getCols());
    
    size_t rowsPerThread = A.getRows() / numThreads;
    
    // Створення та запуск потоків
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startRow = t * rowsPerThread;
        size_t endRow = (t == numThreads - 1) ? A.getRows() : (t + 1) * rowsPerThread;
        
        threads.push_back(std::thread(multiplyMatricesPart, std::ref(A), std::ref(B), std::ref(C), startRow, endRow));
    }
    
    for (size_t t = 0; t < threads.size(); ++t) {
        threads[t].join();
    }
    
    return C;
}

double measureMultiplicationTime(const Matrix& A, const Matrix& B, size_t numThreads) {
    auto start = std::chrono::high_resolution_clock::now();
    
    Matrix C = multiplyMatrices(A, B, numThreads);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    
    return duration.count();
}

void saveTimingData(const std::vector<size_t>& threadCounts, const std::vector<double>& executionTimes) {
    std::ofstream dataFile("timing_data.txt");
    if (!dataFile.is_open()) {
        std::cerr << "Помилка створення файлу даних для часу виконання." << std::endl;
        return;
    }
    
    dataFile << "# Потоків\tЧас (мс)\n";
    for (size_t i = 0; i < threadCounts.size(); ++i) {
        dataFile << threadCounts[i] << "\t" << std::fixed << std::setprecision(3) << executionTimes[i] << std::endl;
    }
    dataFile.close();
    
    std::ofstream scriptFile("plot_script.gp");
    if (!scriptFile.is_open()) {
        std::cerr << "Помилка створення скрипту для gnuplot." << std::endl;
        return;
    }
    
    scriptFile << "set terminal png size 800,600\n";
    scriptFile << "set output 'timing_graph.png'\n";
    scriptFile << "set title 'Залежність часу виконання від кількості потоків'\n";
    scriptFile << "set xlabel 'Кількість потоків'\n";
    scriptFile << "set ylabel 'Час виконання (мс)'\n";
    scriptFile << "set grid\n";
    scriptFile << "plot 'timing_data.txt' using 1:2 with linespoints lw 2 pt 7 title 'Час виконання'\n";
    scriptFile.close();
    
    std::cout << "Створено скрипт plot_script.gp для gnuplot" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3 && argc != 4) {
        std::cout << "Використання: " << argv[0] << " <матриця_A.txt> <матриця_B.txt> [кількість_потоків]" << std::endl;
        return 1;
    }
    
    std::string matrixA_file = argv[1];
    std::string matrixB_file = argv[2];
    
    size_t max_threads = std::thread::hardware_concurrency();
    size_t numThreads = (argc == 4) ? std::stoi(argv[3]) : max_threads;
    
    if (numThreads == 0) {
        numThreads = max_threads;
    }
    
    Matrix A, B;
    
    std::thread loadA(loadMatrixFromFile, std::ref(A), matrixA_file);
    std::thread loadB(loadMatrixFromFile, std::ref(B), matrixB_file);
    
    loadA.join();
    loadB.join();
    
    if (A.getCols() != B.getRows()) {
        std::cerr << "Помилка: Неможливо помножити матриці. Несумісні розміри." << std::endl;
        return 1;
    }

    std::cout << "Обчислення множення матриць розміром " << A.getRows() << "x" << A.getCols() 
              << " та " << B.getRows() << "x" << B.getCols() << std::endl;
    
    std::vector<size_t> threadCounts;
    std::vector<double> executionTimes;
    
    for (size_t t = 1; t <= numThreads; ++t) {
        std::cout << "Тестування з " << t << " потоками..." << std::endl;
        
        const int NUM_RUNS = 3;
        double totalTime = 0.0;
        
        for (int run = 0; run < NUM_RUNS; ++run) {
            double time = measureMultiplicationTime(A, B, t);
            totalTime += time;
        }
        
        double avgTime = totalTime / NUM_RUNS;
        
        threadCounts.push_back(t);
        executionTimes.push_back(avgTime);
        
        std::cout << "Потоків: " << t << ", Час: " << std::fixed << std::setprecision(3) << avgTime << " мс" << std::endl;
    }
    
    std::cout << "Виконання множення з " << numThreads << " потоками..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    Matrix C = multiplyMatrices(A, B, numThreads);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Час виконання: " << duration.count() << " мс" << std::endl;
    
    C.saveToFile("result_matrix.txt");
    std::cout << "Результат збережено у файлі: result_matrix.txt" << std::endl;
    
    saveTimingData(threadCounts, executionTimes);
    
    return 0;
}
