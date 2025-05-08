#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <numeric>
#include <limits>

std::mutex cout_mutex;

struct TextStatistics {
    std::string filename;
    size_t total_chars;
    size_t letters;
    size_t digits;
    size_t spaces;
    size_t punctuation;
    size_t words;
    size_t paragraphs;
    size_t lines;
    
    TextStatistics() : total_chars(0), letters(0), digits(0), spaces(0), 
                      punctuation(0), words(0), paragraphs(0), lines(0) {}
};

struct NumberStatistics {
    std::string filename;
    double mean;
    double min;
    double max;
    size_t count;
    size_t positive_count;
    size_t negative_count;
    size_t zero_count;
    
    NumberStatistics() : mean(0.0), min(std::numeric_limits<double>::max()), 
                        max(std::numeric_limits<double>::lowest()), count(0),
                        positive_count(0), negative_count(0), zero_count(0) {}
};

bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.good();
}

void processTextFile(const std::string& filename) {
    TextStatistics stats;
    stats.filename = filename;
    
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Помилка відкриття файлу: " << filename << std::endl;
        return;
    }
    
    std::string line;
    bool in_paragraph = false;
    
    while (std::getline(file, line)) {
        stats.lines++;
        stats.total_chars += line.length();
        
        if (!line.empty()) {
            if (!in_paragraph) {
                in_paragraph = true;
                stats.paragraphs++;
            }
        } else {
            in_paragraph = false;
        }
        
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            if (std::isalpha(c)) stats.letters++;
            else if (std::isdigit(c)) stats.digits++;
            else if (std::isspace(c)) stats.spaces++;
            else if (std::ispunct(c)) stats.punctuation++;
        }
        
        std::istringstream iss(line);
        std::string word;
        while (iss >> word) {
            stats.words++;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n=== Статистика текстового файлу: " << stats.filename << " ===\n";
        std::cout << "Загальна кількість символів: " << stats.total_chars << std::endl;
        std::cout << "Літери: " << stats.letters << std::endl;
        std::cout << "Цифри: " << stats.digits << std::endl;
        std::cout << "Пробіли: " << stats.spaces << std::endl;
        std::cout << "Знаки пунктуації: " << stats.punctuation << std::endl;
        std::cout << "Кількість слів: " << stats.words << std::endl;
        std::cout << "Кількість абзаців: " << stats.paragraphs << std::endl;
        std::cout << "Кількість рядків: " << stats.lines << std::endl;
    }
    
    file.close();
}

void processNumberFile(const std::string& filename) {
    NumberStatistics stats;
    stats.filename = filename;
    
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Помилка відкриття файлу: " << filename << std::endl;
        return;
    }
    
    std::vector<double> numbers;
    double number;
    
    while (file >> number) {
        numbers.push_back(number);
        stats.count++;
        
        if (number > 0) stats.positive_count++;
        else if (number < 0) stats.negative_count++;
        else stats.zero_count++;
        
        stats.min = std::min(stats.min, number);
        stats.max = std::max(stats.max, number);
    }
    
    if (!numbers.empty()) {
        stats.mean = std::accumulate(numbers.begin(), numbers.end(), 0.0) / numbers.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n=== Статистика числового файлу: " << stats.filename << " ===\n";
        std::cout << "Кількість чисел: " << stats.count << std::endl;
        if (stats.count > 0) {
            std::cout << "Середнє значення: " << stats.mean << std::endl;
            std::cout << "Мінімальне значення: " << stats.min << std::endl;
            std::cout << "Максимальне значення: " << stats.max << std::endl;
        }
        std::cout << "Додатних чисел: " << stats.positive_count << std::endl;
        std::cout << "Від'ємних чисел: " << stats.negative_count << std::endl;
        std::cout << "Нульових значень: " << stats.zero_count << std::endl;
    }
    
    file.close();
}

bool isNumberFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    std::string content;
    std::string line;
    
    while (std::getline(file, line)) {
        if (!content.empty()) content += "\n";
        content += line;
    }
    
    if (content.empty()) {
        return false;
    }
    
    file.close();
    file.open(filename.c_str());
    
    size_t total_chars = content.length();
    size_t digits = 0;
    size_t letters = 0;
    size_t numeric_chars = 0;
    
    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];
        if (std::isdigit(c)) {
            digits++;
            numeric_chars++;
        } else if (std::isalpha(c)) {
            letters++;
        } else if (c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
            numeric_chars++;
        }
    }
    
    bool can_parse_as_numbers = true;
    std::istringstream iss(content);
    double value;
    
    int numbers_parsed = 0;
    while (iss >> value) {
        numbers_parsed++;
    }
    
    bool has_significant_numbers = (digits > 0 && letters < digits * 0.1);
    
    bool numeric_content_dominates = (numeric_chars > (total_chars - numeric_chars - content.find_first_not_of(" \t\n\r")));
    
    return (has_significant_numbers || numeric_content_dominates) && numbers_parsed > 0;
}

void printUsage(const std::string& programName) {
    std::cout << "Використання: " << programName << " [файл1] [файл2] ...\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::vector<std::string> filenames;
    
    for (int i = 1; i < argc; ++i) {
        std::string filename = argv[i];
        
        if (fileExists(filename)) {
            filenames.push_back(filename);
        } else {
            std::cerr << "Файл не існує: " << filename << std::endl;
        }
    }
    
    if (filenames.empty()) {
        std::cerr << "Немає допустимих файлів для обробки. Програма завершується.\n";
        return 1;
    }
    
    std::cout << "Початок обробки " << filenames.size() << " файлів...\n";
    
    std::vector<std::thread> threads;
    
    for (size_t i = 0; i < filenames.size(); ++i) {
        const std::string& file = filenames[i];
        if (isNumberFile(file)) {
            threads.push_back(std::thread(processNumberFile, file));
        } else {
            threads.push_back(std::thread(processTextFile, file));
        }
    }
    
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    
    
    return 0;
}
