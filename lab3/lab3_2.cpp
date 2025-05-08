#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <future>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

using namespace std;
using namespace std::chrono;

class Matrix {
private:
    vector<vector<double>> data;
    size_t rows, cols;

public:
    Matrix(size_t r, size_t c) : rows(r), cols(c) {
        data.resize(rows, vector<double>(cols, 0.0));
    }

    size_t getRows() const { return rows; }
    size_t getCols() const { return cols; }

    vector<double>& operator[](size_t i) { return data[i]; }
    const vector<double>& operator[](size_t i) const { return data[i]; }

    Matrix operator*(const Matrix& other) const {
        if (cols != other.rows) {
            throw invalid_argument("Розміри матриць не підходять для множення");
        }

        Matrix result(rows, other.cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < other.cols; ++j) {
                for (size_t k = 0; k < cols; ++k) {
                    result[i][j] += data[i][k] * other[k][j];
                }
            }
        }
        return result;
    }

    void saveToStream(ostream& stream, const string& name = "") const {
        if (!name.empty()) {
            stream << name << " (" << rows << "x" << cols << "):" << endl;
        }
        
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                stream << setw(10) << fixed << setprecision(0) << data[i][j] << " ";
            }
            stream << endl;
        }
        stream << endl;
    }

    static Matrix loadFromFile(const string& filename) {
        ifstream file(filename);
        if (!file) {
            cerr << "Не вдалося відкрити файл для читання: " << filename << endl;
            exit(1);
        }

        size_t size;
        file >> size;

        Matrix result(size, size);
        
        for (size_t i = 0; i < size; ++i) {
            for (size_t j = 0; j < size; ++j) {
                if (!(file >> result[i][j])) {
                    cerr << "Помилка читання даних з файлу: " << filename << endl;
                    exit(1);
                }
            }
        }
        
        return result;
    }
};

// Послідовне обчислення (A*B)*(C*D)
pair<Matrix, double> sequentialMultiply(const Matrix& A, const Matrix& B, const Matrix& C, const Matrix& D) {
    auto start = high_resolution_clock::now();
    
    Matrix AB = A * B;
    Matrix CD = C * D;
    Matrix result = AB * CD;
    
    auto end = high_resolution_clock::now();
    double duration = duration_cast<milliseconds>(end - start).count() / 1000.0;
    
    return {result, duration};
}

// Асинхронне обчислення (A*B)*(C*D)
pair<Matrix, double> asyncMultiply(const Matrix& A, const Matrix& B, const Matrix& C, const Matrix& D) {
    auto start = high_resolution_clock::now();
    
    future<Matrix> futureAB = async(launch::async, [&]() {
        return A * B;
    });
    
    future<Matrix> futureCD = async(launch::async, [&]() {
        return C * D;
    });
    
    Matrix AB = futureAB.get();
    Matrix CD = futureCD.get();
    
    Matrix result = AB * CD;
    
    auto end = high_resolution_clock::now();
    double duration = duration_cast<milliseconds>(end - start).count() / 1000.0;
    
    return {result, duration};
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Використання: " << argv[0] << " файл_A файл_B файл_C файл_D" << endl;
        return 1;
    }

    string fileA = argv[1];
    string fileB = argv[2];
    string fileC = argv[3];
    string fileD = argv[4];
    
    cout << "Завантаження матриць з файлів..." << endl;
    Matrix A = Matrix::loadFromFile(fileA);
    Matrix B = Matrix::loadFromFile(fileB);
    Matrix C = Matrix::loadFromFile(fileC);
    Matrix D = Matrix::loadFromFile(fileD);
    
    cout << "Розмір матриць: " << A.getRows() << "x" << A.getCols() << endl;
    
    cout << "Виконання послідовного множення..." << endl;
    auto [seqResult, seqTime] = sequentialMultiply(A, B, C, D);
    
    cout << "Виконання асинхронного множення..." << endl;
    auto [asyncResult, asyncTime] = asyncMultiply(A, B, C, D);
    
    // Вивід результатів
    cout << "\nРезультати виконання:" << endl;
    cout << "-------------------------------" << endl;
    cout << "Час послідовного множення: " << fixed << setprecision(3) << seqTime << " с" << endl;
    cout << "Час асинхронного множення: " << fixed << setprecision(3) << asyncTime << " с" << endl;
    
    ofstream report("result.txt");
    if (report) {
        report << "Розмір матриць: " << A.getRows() << "x" << A.getCols() << endl;
        report << "Час послідовного множення: " << fixed << setprecision(3) << seqTime << " с" << endl;
        report << "Час асинхронного множення: " << fixed << setprecision(3) << asyncTime << " с" << endl;
        
        report << "\nМатриця результату (A*B)*(C*D)" << endl;
        asyncResult.saveToStream(report);
    }   
    return 0;
}
