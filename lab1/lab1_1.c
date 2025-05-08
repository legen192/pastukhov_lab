#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <float.h>
#include <unistd.h>

#define MAX_FILENAME_LEN 256
#define MAX_LINE_LEN 1024
#define MAX_FILES 10

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    long characters;
    long words;
    long punctuation;
    long paragraphs;
    long lines;
} TextStats;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    double sum;
    double average;
    double max;
    double min;
    long count;
    long positive_count;
    long negative_count;
    long zero_count;
} NumberStats;

bool is_punctuation(char c) {
    return (c == '.' || c == ',' || c == ';' || c == ':' || 
            c == '!' || c == '?' || c == '-' || c == '(' || 
            c == ')' || c == '[' || c == ']' || c == '"' ||
            c == '\'' || c == '/' || c == '_');
}

void* process_text_file(void* arg) {
    char* filename = (char*)arg;
    TextStats* stats = (TextStats*)malloc(sizeof(TextStats));
    
    if (stats == NULL) {
        fprintf(stderr, "Помилка виділення пам'яті\n");
        pthread_exit(NULL);
    }
    
    strncpy(stats->filename, filename, MAX_FILENAME_LEN - 1);
    stats->filename[MAX_FILENAME_LEN - 1] = '\0';
    stats->characters = 0;
    stats->words = 0;
    stats->punctuation = 0;
    stats->paragraphs = 0;
    stats->lines = 0;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Не вдається відкрити файл: %s\n", filename);
        pthread_mutex_unlock(&print_mutex);
        free(stats);
        pthread_exit(NULL);
    }
    
    char line[MAX_LINE_LEN];
    bool in_word = false;
    bool was_empty_line = false;
    
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        stats->lines++;
        
        if (strlen(line) <= 1) { 
            was_empty_line = true;
            continue;
        } else if (was_empty_line) {
            stats->paragraphs++;
            was_empty_line = false;
        }
        
        for (int i = 0; line[i] != '\0'; i++) {
            if (!isspace(line[i])) {
                stats->characters++;
                
                if (is_punctuation(line[i])) {
                    stats->punctuation++;
                }
            }
            
            if (isspace(line[i])) {
                if (in_word) {
                    in_word = false;
                }
            } else {
                if (!in_word) {
                    in_word = true;
                    stats->words++;
                }
            }
        }
        
        in_word = false;
    }
    
    if (stats->lines > 0 && stats->paragraphs == 0) {
        stats->paragraphs = 1;
    }
    
    fclose(file);
    
    pthread_mutex_lock(&print_mutex);
    printf("\n=== Статистика текстового файлу [%s] ===\n", stats->filename);
    printf("Кількість символів: %ld\n", stats->characters);
    printf("Кількість слів: %ld\n", stats->words);
    printf("Кількість знаків пунктуації: %ld\n", stats->punctuation);
    printf("Кількість абзаців: %ld\n", stats->paragraphs);
    printf("Кількість рядків: %ld\n", stats->lines);
    pthread_mutex_unlock(&print_mutex);
    
    pthread_exit((void*)stats);
}

void* process_number_file(void* arg) {
    char* filename = (char*)arg;
    NumberStats* stats = (NumberStats*)malloc(sizeof(NumberStats));
    
    if (stats == NULL) {
        fprintf(stderr, "Помилка виділення пам'яті\n");
        pthread_exit(NULL);
    }
    
    strncpy(stats->filename, filename, MAX_FILENAME_LEN - 1);
    stats->filename[MAX_FILENAME_LEN - 1] = '\0';
    stats->sum = 0.0;
    stats->count = 0;
    stats->positive_count = 0;
    stats->negative_count = 0;
    stats->zero_count = 0;
    stats->max = -DBL_MAX;
    stats->min = DBL_MAX;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Не вдається відкрити файл: %s\n", filename);
        pthread_mutex_unlock(&print_mutex);
        free(stats);
        pthread_exit(NULL);
    }
    
    double value;
    char line[MAX_LINE_LEN];
    
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        char* token = strtok(line, " \t\n");
        
        while (token != NULL) {
            if (sscanf(token, "%lf", &value) == 1) {
                stats->sum += value;
                stats->count++;
                
                if (value > stats->max) {
                    stats->max = value;
                }
                
                if (value < stats->min) {
                    stats->min = value;
                }
                
                if (value > 0) {
                    stats->positive_count++;
                } else if (value < 0) {
                    stats->negative_count++;
                } else {
                    stats->zero_count++;
                }
            }
            
            token = strtok(NULL, " \t\n");
        }
    }
    
    fclose(file);
    
    if (stats->count > 0) {
        stats->average = stats->sum / stats->count;
    } else {
        stats->average = 0.0;
    }
    
    pthread_mutex_lock(&print_mutex);
    printf("\n=== Статистика числового файлу [%s] ===\n", stats->filename);
    printf("Кількість чисел: %ld\n", stats->count);
    printf("Сума чисел: %f\n", stats->sum);
    printf("Середнє значення: %f\n", stats->average);
    printf("Максимальне значення: %f\n", stats->max);
    printf("Мінімальне значення: %f\n", stats->min);
    printf("Кількість додатніх чисел: %ld\n", stats->positive_count);
    printf("Кількість від'ємних чисел: %ld\n", stats->negative_count);
    printf("Кількість нулів: %ld\n", stats->zero_count);
    pthread_mutex_unlock(&print_mutex);
    
    pthread_exit((void*)stats);
}

int detect_file_type(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Не вдається відкрити файл для визначення типу: %s\n", filename);
        return -1; 
    }
    
    char line[MAX_LINE_LEN];
    if (fgets(line, MAX_LINE_LEN, file) != NULL) {
        int digit_count = 0;
        int char_count = 0;
        
        for (int i = 0; line[i] != '\0'; i++) {
            if (isdigit(line[i]) || line[i] == '.' || line[i] == '-' || line[i] == '+' || 
                isspace(line[i])) {
                if (isdigit(line[i])) {
                    digit_count++;
                }
            } else {
                char_count++;
            }
        }
        
        fclose(file);
        
        if (digit_count > char_count) {
            return 1; 
        } else {
            return 0; 
        }
    }
    
    fclose(file);
    return -1; 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Використання: %s <файл1> <файл2> ... <файлN>\n", argv[0]);
        return 1;
    }
    
    int file_count = argc - 1;
    if (file_count > MAX_FILES) {
        printf("Перевищено максимальну кількість файлів (%d). Буде оброблено тільки перші %d файлів.\n", 
               MAX_FILES, MAX_FILES);
        file_count = MAX_FILES;
    }
    
    pthread_t threads[MAX_FILES];
    int file_types[MAX_FILES]; 
    void* thread_results[MAX_FILES];
    
    printf("Початок обробки %d файлів...\n", file_count);
    
    for (int i = 0; i < file_count; i++) {
        file_types[i] = detect_file_type(argv[i + 1]);
        
        if (file_types[i] == 0) { 
            pthread_create(&threads[i], NULL, process_text_file, argv[i + 1]);
            printf("Створено потік #%d для обробки текстового файлу: %s\n", i, argv[i + 1]);
        } else if (file_types[i] == 1) { 
            pthread_create(&threads[i], NULL, process_number_file, argv[i + 1]);
            printf("Створено потік #%d для обробки числового файлу: %s\n", i, argv[i + 1]);
        } else {
            printf("Пропускаємо файл #%d: %s (не вдалося визначити тип)\n", i, argv[i + 1]);
            threads[i] = 0; 
        }
    }
    
    for (int i = 0; i < file_count; i++) {
        if (threads[i] != 0) {
            pthread_join(threads[i], &thread_results[i]);
            printf("Потік #%d завершив обробку\n", i);
        }
    }
    
    for (int i = 0; i < file_count; i++) {
        if (thread_results[i] != NULL) {
            free(thread_results[i]);
        }
    }
    
    pthread_mutex_destroy(&print_mutex);
    
    printf("\nОбробка всіх файлів завершена.\n");
    
    return 0;
}
