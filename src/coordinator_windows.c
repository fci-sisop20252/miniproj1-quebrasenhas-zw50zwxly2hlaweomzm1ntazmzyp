#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#endif
#include "hash_utils.h"

/**
 * WINDOWS-COMPATIBLE VERSION - Mini-Projeto 1: Quebra de Senhas
 * Uses threads instead of processes for Windows compatibility
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

// Thread data structure for Windows
typedef struct {
    char target_hash[33];
    char start_password[11];
    char end_password[11];
    char charset[256];
    int password_len;
    int worker_id;
} WorkerData;

// global flag for early termination
volatile int password_found = 0;

/**
 * Calcula o tamanho total do espaço de busca
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Incrementa senha (copiado do worker.c)
 */
int find_index(char c, const char *charset, int charset_len) {
    for (int i = 0; i < charset_len; i++) {
        if (charset[i] == c) {
            return i;
        }
    }
    return -1;
}

int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    for (int i = password_len - 1; i >= 0; i--) {
        int index = find_index(password[i], charset, charset_len);
        if (index == -1) return 0; 
        index++;
        if (index < charset_len) {
            password[i] = charset[index];
            for (int j = i + 1; j < password_len; j++) {
                password[j] = charset[0];
            }
            return 1;
        } else {
            password[i] = charset[0];
        }
    }
    return 0;
}

/**
 * Salva resultado de forma thread-safe
 */
void save_result(int worker_id, const char *password) {
    if (password_found) return; // Já encontrado
    
#ifdef _WIN32
    FILE *f = fopen(RESULT_FILE, "w");
    if (f) {
        fprintf(f, "%d:%s\n", worker_id, password);
        fclose(f);
        password_found = 1;
    }
#else
    int fd = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd != -1) {
        char buffer[100];
        int n = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
        write(fd, buffer, n);
        close(fd);
        password_found = 1;
    }
#endif
}

/**
 * Thread function for password search
 */
#ifdef _WIN32
unsigned __stdcall worker_thread(void* data) {
#else
void* worker_thread(void* data) {
#endif
    WorkerData* wd = (WorkerData*)data;
    
    printf("[Worker %d] Iniciado: %s ate %s\n", wd->worker_id, wd->start_password, wd->end_password);
    
    char current_password[11];
    strcpy(current_password, wd->start_password);
    
    char computed_hash[33];
    long long passwords_checked = 0;
    
    while (!password_found) {
        // hashing function
        md5_string(current_password, computed_hash);
        
        // compare digest
        if (strcmp(computed_hash, wd->target_hash) == 0) {
            printf("[Worker %d] SENHA ENCONTRADA: %s\n", wd->worker_id, current_password);
            save_result(wd->worker_id, current_password);
            break;
        }
        
        // check end of interval
        if (strcmp(current_password, wd->end_password) >= 0) {
            break;
        }
        
        // password increment
        if (!increment_password(current_password, wd->charset, strlen(wd->charset), wd->password_len)) {
            break;
        }
        
        passwords_checked++;
        
        // concurrency check, 10k passwords OR found by other worker
        if (passwords_checked % 10000 == 0 && password_found) {
            break;
        }
    }
    
    printf("[Worker %d] Finalizado. Total: %lld senhas\n", wd->worker_id, passwords_checked);
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/**
 * main
 */
int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        printf("Exemplo: %s \"900150983cd24fb0d6963f7d28e17f72\" 3 \"abc\" 2\n", argv[0]);
        return 1;
    }
    
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // error handling: lenght, null, and worker limit
    if (password_len < 1 || password_len > 10) {
        printf("Erro: tamanho de senha deve estar entre 1 e 10\n");
        return 1;
    }
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        printf("Erro: numero de workers deve estar entre 1 e %d\n", MAX_WORKERS);
        return 1;
    }
    if (charset_len == 0) {
        printf("Erro: charset nao pode ser vazio\n");
        return 1;
    }
    
    printf("=== Mini-Projeto 1: Quebra de Senhas (Windows Version) ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Numero de workers: %d\n", num_workers);
    
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaco de busca total: %lld combinacoes\n\n", total_space);
    
    // remove previous result file
    remove(RESULT_FILE);
    password_found = 0;
    
    time_t start_time = time(NULL);
    
    // spawn workers
    WorkerData workers_data[MAX_WORKERS];
    
#ifdef _WIN32
    HANDLE threads[MAX_WORKERS];
#else
    pthread_t threads[MAX_WORKERS];
#endif
    
    long long passwords_per_worker = total_space / num_workers;
    long long remaining = total_space % num_workers;
    
    printf("Iniciando workers...\n");
    
    for (int i = 0; i < num_workers; i++) {
        // worker interval calculation
        long long start_index = i * passwords_per_worker;
        if (i < remaining) {
            start_index += i;
        } else {
            start_index += remaining;
        }
        
        long long worker_passwords = passwords_per_worker;
        if (i < remaining) {
            worker_passwords++;
        }
        
        long long end_index = start_index + worker_passwords - 1;
        
        // worker data config
        strcpy(workers_data[i].target_hash, target_hash);
        strcpy(workers_data[i].charset, charset);
        workers_data[i].password_len = password_len;
        workers_data[i].worker_id = i;
        
        index_to_password(start_index, charset, charset_len, password_len, workers_data[i].start_password);
        index_to_password(end_index, charset, charset_len, password_len, workers_data[i].end_password);
        
        printf("Worker %d: %s ate %s (%lld senhas)\n", i, 
               workers_data[i].start_password, workers_data[i].end_password, worker_passwords);
        
        // spawn thread
#ifdef _WIN32
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker_thread, &workers_data[i], 0, NULL);
#else
        pthread_create(&threads[i], NULL, worker_thread, &workers_data[i]);
#endif
    }
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusao...\n");
    
    // wait for threads
    for (int i = 0; i < num_workers; i++) {
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
#else
        pthread_join(threads[i], NULL);
#endif
    }
    
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // Verificar resultado
    FILE *result_file = fopen(RESULT_FILE, "r");
    if (result_file != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), result_file) != NULL) {
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                *colon = '\0';
                int found_worker_id = atoi(line);
                char *found_password = colon + 1;
                
                // Remover newline
                char *newline = strchr(found_password, '\n');
                if (newline != NULL) {
                    *newline = '\0';
                }
                
                char computed_hash[33];
                md5_string(found_password, computed_hash);
                
                printf("SUCCESS: SENHA ENCONTRADA!\n");
                printf("Worker ID: %d\n", found_worker_id);
                printf("Senha: %s\n", found_password);
                printf("Hash calculado: %s\n", computed_hash);
                printf("Hash alvo:      %s\n", target_hash);
                
                if (strcmp(computed_hash, target_hash) == 0) {
                    printf("SUCCESS: Hash verificado com sucesso!\n");
                } else {
                    printf("X ERRO: Hash nao confere!\n");
                }
            }
        }
        fclose(result_file);
    } else {
        printf("X Senha nao encontrada no espaco de busca especificado.\n");
    }
    
    printf("\n=== Estatisticas de Performance ===\n");
    printf("Tempo total de execucao: %.2f segundos\n", elapsed_time);
    printf("Espaco de busca total: %lld combinacoes\n", total_space);
    printf("Numero de workers: %d\n", num_workers);
    if (elapsed_time > 0) {
        printf("Taxa de verificacao estimada: %.0f senhas/segundo\n", total_space / elapsed_time);
    }
    
    return 0;
}