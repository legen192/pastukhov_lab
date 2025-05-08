#include <iostream>
#include <fstream>
#include <future>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <locale>
#include <algorithm>

using namespace std;

struct Matrix {
    int size = 0;
    vector<vector<int>> data;
    
    void print() const {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                cout << data[i][j] << "\t";
            }
            cout << endl;
        }
    }
};

Matrix readMatrixFromFile(const string& filename) {
    Matrix matrix;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Помилка відкриття файлу: " << filename << endl;
        return matrix;
    }
    
    file >> matrix.size;
    
    if (file.fail()) {
        cerr << "Помилка читання розміру матриці з файлу: " << filename << endl;
        return matrix;
    }
    
    if (matrix.size <= 0) {
        cerr << "Некоректний розмір матриці у файлі: " << filename << endl;
        return matrix;
    }
    
    // Debug info
    cout << "Розмір матриці з файлу " << filename << ": " << matrix.size << "x" << matrix.size << endl;
    
    matrix.data.resize(matrix.size, vector<int>(matrix.size, 0));
    
    for (int i = 0; i < matrix.size; i++) {
        for (int j = 0; j < matrix.size; j++) {
            if (file.eof()) {
                cerr << "Передчасний кінець файлу при читанні елемента [" << i << "][" << j << "] з файлу: " << filename << endl;
                return matrix;
            }
            
            file >> matrix.data[i][j];
            
            if (file.fail()) {
                file.clear();
                string invalid_value;
                file >> invalid_value;
                cerr << "Некоректне значення на позиції [" << i << "][" << j << "] у файлі: " << filename << endl;
                matrix.data[i][j] = 0;
            }
        }
    }
    
    cout << "Матрицю з файлу " << filename << " зчитано успішно" << endl;
    return matrix;
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "uk_UA.UTF-8");
    if (argc != 5) {
        cerr << "Використання: " << argv[0] << " <файл1> <файл2> <файл3> <файл4>" << endl;
        cerr << "Приклад: " << argv[0] << " matrix1.txt matrix2.txt matrix3.txt matrix4.txt" << endl;
        return 1;
    }
    
    vector<string> filenames;
    for (int i = 1; i < argc; i++) {
        filenames.push_back(argv[i]);
    }
    
    cout << "Запускаємо асинхронне зчитування матриць..." << endl;
    
    vector<future<Matrix>> futures;
    
    auto startTime = chrono::high_resolution_clock::now();
    
    for (const auto& filename : filenames) {
        futures.push_back(async(launch::async, readMatrixFromFile, filename));
    }
    
    cout << "Всі асинхронні завдання запущено" << endl;
    
    vector<Matrix> matrices(futures.size());
    vector<bool> completed(futures.size(), false);
    bool allCompleted = false;
    
    while (!allCompleted) {
        allCompleted = true;
        
        for (size_t i = 0; i < futures.size(); i++) {
            if (!completed[i]) {
                auto status = futures[i].wait_for(chrono::seconds(0));
                
                if (status == future_status::ready) {
                    matrices[i] = futures[i].get();
                    completed[i] = true;
                    cout << "Завершено зчитування матриці з файлу " << filenames[i] << endl;
                } else {
                    allCompleted = false;
                }
            }
        }
        
        if (!allCompleted) {
            this_thread::yield();
        }
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    
    cout << "\nВсі матриці успішно зчитано за " << duration << " мс" << endl;
    
    for (size_t i = 0; i < matrices.size(); i++) {
        cout << "\nМатриця " << i + 1 << " з файлу " << filenames[i] << " (" 
             << matrices[i].size << "x" << matrices[i].size << "):" << endl;
        
        if (matrices[i].size > 0) {
            int displaySize = min(matrices[i].size, 10);
            for (int r = 0; r < displaySize; r++) {
                for (int c = 0; c < displaySize; c++) {
                    cout << matrices[i].data[r][c] << "\t";
                }
                cout << endl;
            }
            
            if (matrices[i].size > 10) {
                cout << "... (відображено лише перші 10x10 елементів)" << endl;
            }
        } else {
            cout << "Матриця порожня або невалідна" << endl;
        }
    }
    
    return 0;
}
