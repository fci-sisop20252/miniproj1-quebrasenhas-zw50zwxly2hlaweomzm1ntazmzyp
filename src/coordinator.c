#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
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
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
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
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    
    // IMPLEMENTE AQUI: verificação de argc e mensagem de erro
    if (argc != 5) {
        printf("Deu erro,numero de args errado\n");
        return 1;
    }
    
    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // TODO: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    if (password_len < 1 || password_len > 10) {
        printf("Deu erro,tamanho de senha esta invalido\n");
        return 1;
    }

    // - num_workers deve estar entre 1 e MAX_WORKERS
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        printf("Deu erro,numero de workers esta invalido\n");
        return 1;
    }
    // - charset not null/empty
    if (charset_len == 0) {
        printf("Deu erro,charset esta vazio\n");
        return 1;
    }

    
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);
    
    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);
    
    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);
    
    // Registrar tempo de início
    time_t start_time = time(NULL);
    
    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers

    
    // IMPLEMENTE AQUI:
    long long passwords_per_worker = total_space / num_workers;
    long long remaining = total_space % num_workers;
    
    
    // Arrays para armazenar PIDs dos workers
    pid_t workers[MAX_WORKERS];
    
    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando workers...\n");


    // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
        // Calcular intervalo de senhas para este worker
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
        
        // Converter indices para senhas de inicio e fim
        char start_password[11], end_password[11];
        index_to_password(start_index, charset, charset_len, password_len, start_password);
        index_to_password(end_index, charset, charset_len, password_len, end_password);
        
        printf("Worker %d: %s até %s (%lld senhas)\n", i, start_password, end_password, worker_passwords);
        
        // TODO 4: Usar fork() para criar processo filho
        pid_t pid = fork();
        
        if (pid == -1) {
            // TODO 7: Tratar erros de fork()
            perror("Erro ao criar processo worker");
            exit(1);
        } else if (pid == 0) {
            // TODO 6: No processo filho: usar execl() para executar worker
            char worker_id_str[10], password_len_str[10];
            snprintf(worker_id_str, sizeof(worker_id_str), "%d", i);
            snprintf(password_len_str, sizeof(password_len_str), "%d", password_len);
            
            execl("./worker", "worker", target_hash, start_password, end_password, 
                  charset, password_len_str, worker_id_str, NULL);
            
            // TODO 7: Tratar erros de execl()
            perror("Erro ao executar worker");
            exit(1);
        } else {
            // TODO 5: No processo pai: armazenar PID
            workers[i] = pid;
        }
    }
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    
    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    
    // IMPLEMENTE AQUI:
    // - Loop para aguardar cada worker terminar
    // - Usar wait() para capturar status de saída
    // - Identificar qual worker terminou
    // - Verificar se terminou normalmente ou com erro
    // - Contar quantos workers terminaram
    
    int workers_finished = 0;
    for (int i = 0; i < num_workers; i++) {
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid == -1) {
            perror("Erro ao aguardar worker");
            continue;
        }
        
        // Identificar qual worker terminou
        int worker_id = -1;
        for (int j = 0; j < num_workers; j++) {
            if (workers[j] == finished_pid) {
                worker_id = j;
                break;
            }
        }
        
        if (WIFEXITED(status)) {
            printf("Worker %d (PID %d) terminou normalmente com código %d\n", 
                   worker_id, finished_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Worker %d (PID %d) terminou por sinal %d\n", 
                   worker_id, finished_pid, WTERMSIG(status));
        }
        
        workers_finished++;
    }
    
    printf("Todos os %d workers terminaram.\n", workers_finished);
    
    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // TODO 9: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir
    
    // IMPLEMENTE AQUI:
    // - Abrir arquivo RESULT_FILE para leitura
    // - Ler conteúdo do arquivo
    // - Fazer parse do formato "worker_id:password"
    // - Verificar o hash usando md5_string()
    // - Exibir resultado encontrado
    
    FILE *result_file = fopen(RESULT_FILE, "r");
    if (result_file != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), result_file) != NULL) {
            // Parse do formato "worker_id:password\n"
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                *colon = '\0';
                int found_worker_id = atoi(line);
                char *found_password = colon + 1;
                
                // Remover newline se presente
                char *newline = strchr(found_password, '\n');
                if (newline != NULL) {
                    *newline = '\0';
                }
                
                // Verificar o hash usando md5_string()
                char computed_hash[33];
                md5_string(found_password, computed_hash);
                
                printf("✓ SENHA ENCONTRADA!\n");
                printf("Worker ID: %d\n", found_worker_id);
                printf("Senha: %s\n", found_password);
                printf("Hash calculado: %s\n", computed_hash);
                printf("Hash alvo:      %s\n", target_hash);
                
                if (strcmp(computed_hash, target_hash) == 0) {
                    printf("✓ Hash verificado com sucesso!\n");
                } else {
                    printf("✗ ERRO: Hash não confere!\n");
                }
            }
        }
        fclose(result_file);
    } else {
        printf("✗ Senha não encontrada no espaço de busca especificado.\n");
        printf("  Verifique se o hash está correto e se o charset contém todos os caracteres.\n");
    }
    
    // Estatísticas finais (opcional)
    // TODO: Calcular e exibir estatísticas de performance
    
    printf("\n=== Estatísticas de Performance ===\n");
    printf("Tempo total de execução: %.2f segundos\n", elapsed_time);
    printf("Espaço de busca total: %lld combinações\n", total_space);
    printf("Número de workers: %d\n", num_workers);
    if (elapsed_time > 0) {
        printf("Taxa de verificação estimada: %.0f senhas/segundo\n", total_space / elapsed_time);
        printf("Speedup teórico com %d workers: %.1fx\n", num_workers, (double)num_workers);
    }
    
    return 0;
}