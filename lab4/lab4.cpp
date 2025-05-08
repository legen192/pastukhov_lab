#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono.hpp>
#include <fstream>
#include <string>
#include <sstream>

struct Matrix {
    std::vector<std::vector<double>> data;
    size_t rows;
    size_t cols;

    Matrix(size_t rows, size_t cols) : rows(rows), cols(cols) {
        data.resize(rows, std::vector<double>(cols, 0.0));
    }

    void randomize() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(1.0, 10.0);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = dis(gen);
            }
        }
    }

    void print(const std::string& name) const {
        std::cout << "Матриця " << name << " (" << rows << "x" << cols << "):" << std::endl;
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                std::cout << std::fixed << std::setprecision(2) << std::setw(8) << data[i][j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    static Matrix loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Неможливо відкрити файл: " + filename);
        }

        size_t rows, cols;
        std::string first_line;
        std::getline(file, first_line);
        
        std::stringstream ss(first_line);
        if (!(ss >> rows >> cols)) {
            throw std::runtime_error("Некоректний формат розміру матриці в файлі: " + filename + 
                                    ". Перший рядок повинен містити два числа: кількість рядків та стовпців.");
        }
        
        if (rows <= 0 || cols <= 0) {
            throw std::runtime_error("Некоректні розміри матриці в файлі: " + filename + 
                                    ". Розміри повинні бути більше нуля.");
        }

        Matrix matrix(rows, cols);

        for (size_t i = 0; i < rows; ++i) {
            std::string line;
            if (!std::getline(file, line)) {
                throw std::runtime_error("Недостатньо даних в файлі: " + filename + 
                                        ". Очікувалось " + std::to_string(rows) + " рядків.");
            }
            
            std::stringstream line_ss(line);
            for (size_t j = 0; j < cols; ++j) {
                if (!(line_ss >> matrix.data[i][j])) {
                    throw std::runtime_error("Помилка при читанні даних з файлу: " + filename + 
                                            ". Рядок " + std::to_string(i+1) + ", стовпець " + 
                                            std::to_string(j+1) + ". Перевірте формат файлу.");
                }
            }
            
            double extra_data;
            if (line_ss >> extra_data) {
                throw std::runtime_error("Зайві дані в рядку " + std::to_string(i+1) + 
                                        " файлу: " + filename + ". Очікувалось " + 
                                        std::to_string(cols) + " елементів.");
            }
        }
        
        std::string extra_line;
        if (std::getline(file, extra_line) && !extra_line.empty()) {
            throw std::runtime_error("Зайві дані в файлі: " + filename + 
                                    ". Очікувалось " + std::to_string(rows) + " рядків.");
        }

        file.close();
        
        std::cout << "Успішно завантажено матрицю з файлу " << filename 
                  << " розміром " << rows << "x" << cols << std::endl;
                  
        return matrix;
    }
};

bool should_terminate = false;
boost::mutex terminate_mutex;
boost::condition_variable terminate_cv;

Matrix multiply_matrices(const Matrix& A, const Matrix& B) {
    if (A.cols != B.rows) {
        throw std::runtime_error("Неможливо помножити матриці: розміри не співпадають");
    }

    Matrix result(A.rows, B.cols);

    for (size_t i = 0; i < A.rows; ++i) {
        for (size_t j = 0; j < B.cols; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < A.cols; ++k) {
                sum += A.data[i][k] * B.data[k][j];
                
                {
                    boost::lock_guard<boost::mutex> lock(terminate_mutex);
                    if (should_terminate) {
                        throw std::runtime_error("Обчислення було перервано");
                    }
                }
            }
            result.data[i][j] = sum;
        }
    }

    return result;
}

void multiply_matrices_partial(const Matrix& A, const Matrix& B, Matrix& result, 
                               size_t start_row, size_t end_row) {
    for (size_t i = start_row; i < end_row; ++i) {
        for (size_t j = 0; j < B.cols; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < A.cols; ++k) {
                sum += A.data[i][k] * B.data[k][j];
                
                {
                    boost::lock_guard<boost::mutex> lock(terminate_mutex);
                    if (should_terminate) {
                        return;
                    }
                }
            }
            result.data[i][j] = sum;
        }
    }
}

Matrix parallel_multiply_matrices(const Matrix& A, const Matrix& B, int num_threads) {
    if (A.cols != B.rows) {
        throw std::runtime_error("Неможливо помножити матриці: розміри не співпадають");
    }

    Matrix result(A.rows, B.cols);
    std::vector<boost::thread> threads;
    
    size_t rows_per_thread = A.rows / num_threads;
    
    for (int t = 0; t < num_threads; ++t) {
        size_t start_row = t * rows_per_thread;
        size_t end_row = (t == num_threads - 1) ? A.rows : start_row + rows_per_thread;
        
        threads.push_back(boost::thread(
            multiply_matrices_partial, std::ref(A), std::ref(B), std::ref(result), start_row, end_row
        ));
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return result;
}

void interrupt_calculation() {
    {
        boost::lock_guard<boost::mutex> lock(terminate_mutex);
        should_terminate = true;
    }
    terminate_cv.notify_all();
}

void user_input_thread() {
    std::cout << "Натисніть 'q' і Enter для переривання обчислень..." << std::endl;
    char input;
    while (true) {
        std::cin >> input;
        if (input == 'q' || input == 'Q') {
            interrupt_calculation();
            break;
        }
    }
}

std::string get_file_path(const std::string& matrix_name) {
    std::string file_path;
    std::cout << "Введіть шлях до файлу для матриці " << matrix_name << ": ";
    std::cin >> file_path;
    return file_path;
}

void save_matrix_to_file(const Matrix& matrix, const std::string& filename) {
    std::ofstream result_file(filename);
    if (!result_file.is_open()) {
        throw std::runtime_error("Неможливо створити файл: " + filename);
    }
    
    result_file << matrix.rows << " " << matrix.cols << std::endl;
    
    for (size_t i = 0; i < matrix.rows; ++i) {
        for (size_t j = 0; j < matrix.cols; ++j) {
            result_file << std::fixed << std::setprecision(6) << matrix.data[i][j];
            if (j < matrix.cols - 1) {
                result_file << " ";
            }
        }
        result_file << std::endl;
    }
    
    result_file.close();
    std::cout << "Результат збережено у файл: " << filename << std::endl;
}

int main() {
    try {
        std::cout << "Багатопотокове множення матриць з використанням Boost" << std::endl;
        
        Matrix A = Matrix::loadFromFile(get_file_path("A"));
        Matrix B = Matrix::loadFromFile(get_file_path("B"));
        Matrix C = Matrix::loadFromFile(get_file_path("C"));
        Matrix D = Matrix::loadFromFile(get_file_path("D"));
        
        if (A.cols != B.rows) {
            std::stringstream error_msg;
            error_msg << "Неможливо помножити матриці A*B: розміри не співпадають. " 
                      << "A має " << A.cols << " стовпців, а B має " << B.rows 
                      << " рядків. Для можливості множення ці значення повинні бути однаковими.";
            throw std::runtime_error(error_msg.str());
        }
        if (C.cols != D.rows) {
            std::stringstream error_msg;
            error_msg << "Неможливо помножити матриці C*D: розміри не співпадають. " 
                      << "C має " << C.cols << " стовпців, а D має " << D.rows 
                      << " рядків. Для можливості множення ці значення повинні бути однаковими.";
            throw std::runtime_error(error_msg.str());
        }
        
        std::cout << "Завантажено матриці:" << std::endl;
        std::cout << "A: " << A.rows << "x" << A.cols << std::endl;
        std::cout << "B: " << B.rows << "x" << B.cols << std::endl;
        std::cout << "C: " << C.rows << "x" << C.cols << std::endl;
        std::cout << "D: " << D.rows << "x" << D.cols << std::endl;
        
        boost::thread input_thread(user_input_thread);
        
        int num_threads = boost::thread::hardware_concurrency();
        
        auto start_time = boost::chrono::high_resolution_clock::now();
        
        std::cout << "Обчислюємо (AxB)..." << std::endl;
        Matrix AB = parallel_multiply_matrices(A, B, num_threads);
        
        {
            boost::lock_guard<boost::mutex> lock(terminate_mutex);
            if (should_terminate) {
                std::cout << "Обчислення перервано користувачем." << std::endl;
                input_thread.join();
                return 0;
            }
        }
        
        std::cout << "Обчислюємо (CxD)..." << std::endl;
        Matrix CD = parallel_multiply_matrices(C, D, num_threads);
        
        {
            boost::lock_guard<boost::mutex> lock(terminate_mutex);
            if (should_terminate) {
                std::cout << "Обчислення перервано користувачем." << std::endl;
                input_thread.join();
                return 0;
            }
        }
        
        std::cout << "Обчислюємо (AxB)x(CxD)..." << std::endl;
        if (AB.cols != CD.rows) {
            std::stringstream error_msg;
            error_msg << "Неможливо помножити результати (AxB)x(CxD): розміри не співпадають. " 
                      << "Матриця (AxB) має розмір " << AB.rows << "x" << AB.cols 
                      << ", а матриця (CxD) має розмір " << CD.rows << "x" << CD.cols
                      << ". Для можливості множення необхідно, щоб кількість стовпців у (AxB) (" 
                      << AB.cols << ") дорівнювала кількості рядків у (CxD) (" << CD.rows << ").";
            throw std::runtime_error(error_msg.str());
        }
        
        Matrix result = parallel_multiply_matrices(AB, CD, num_threads);
        
        auto end_time = boost::chrono::high_resolution_clock::now();
        auto duration = boost::chrono::duration_cast<boost::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Обчислення завершено!" << std::endl;
        std::cout << "Час виконання: " << duration.count() << " мс" << std::endl;
        std::cout << "Розмір результуючої матриці: " << result.rows << "x" << result.cols << std::endl;
        
        const std::string result_filename = "result.txt";
        save_matrix_to_file(result, result_filename);
        
        interrupt_calculation();
        input_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Помилка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
