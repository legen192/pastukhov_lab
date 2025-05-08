#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <omp.h>

using namespace std;

int main() {
    constexpr int max_iter = 50000;
    vector<pair<int, double>> results;
    
    for (int n = 1; n <= 10; n++) {
        double Sum = 0.0;
        
        auto start = chrono::high_resolution_clock::now();
        
        #pragma omp parallel for num_threads(n)
        for (int i = 1; i <= max_iter; i++) {
            #pragma omp critical
            Sum += 1.0 / (i * i);
        }
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end - start;
        
        cout << "Потоків: " << n << ", Час: " << elapsed.count() 
             << " мс, Сума: " << Sum << endl;
        
        results.push_back({n, elapsed.count()});
    }
    
    ofstream datafile("timing.dat");
    if (datafile.is_open()) {
        datafile << "# Кількість_потоків Час_виконання(мс)\n";
        for (const auto& result : results) {
            datafile << result.first << " " << result.second << "\n";
        }
        datafile.close();
        cout << "Результати збережено у файлі timing.dat" << endl;
    } else {
        cerr << "Не вдалося відкрити файл для запису результатів." << endl;
    }
    
    return 0;
}
