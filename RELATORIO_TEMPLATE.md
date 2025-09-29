# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Nome (Matrícula), Nome (Matrícula),,,  

Pedro Roberto Fernandes Noronha (10443434)
Alice Oliveira Duarte (10419323)
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

O espaço de busca total é calculado como o número de combinações possíveis de senhas. Esse espaço é dividido igualmente entre os workers, usando divisão inteira. O resto da divisão é distribuído entre os primeiros workers. Cada worker recebe um intervalo de índices e converte esses índices em senhas inicial e final para processar.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
long long passwords_per_worker = total_space / num_workers;
long long remaining = total_space % num_workers;

for (int i = 0; i < num_workers; i++) {
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
    // ...conversão dos índices para senha...
}

```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

O coordinator cria os processos dos workers usando um loop com fork(). No processo filho, utiliza execl() para executar o programa worker, passando os argumentos necessários. No processo pai, armazena o PID de cada worker e, utiliza waitpid() para aguardar a finalização de cada worker.
Implementação do waitpid() foi countermeasure temporária antes do pull do merge. Acreditava que era uma diferença entre o macOS e a VM do linux, diferença entre como o wait() funcionava ao invés de esperar por pid.

**Código do fork/exec:**
```c
for (int i = 0; i < num_workers; i++) {
    // ...cálculo dos intervalos...
    pid_t pid = fork();
    if (pid == -1) {
        perror("Erro ao criar processo worker");
        exit(1);
    } else if (pid == 0) {
        char worker_id_str[10], password_len_str[10];
        snprintf(worker_id_str, sizeof(worker_id_str), "%d", i);
        snprintf(password_len_str, sizeof(password_len_str), "%d", password_len);
        execl("./worker", "worker", target_hash, start_password, end_password, 
              charset, password_len_str, worker_id_str, NULL);
        perror("Erro ao executar worker");
        exit(1);
    } else {
        workers[i] = pid;
    }
}
for (int i = 0; i < num_workers; i++) {
    int status;
    pid_t finished_pid = waitpid(workers[i], &status, 0);
    // ...verificação do status...
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**

A escrita do resultado é feita de forma atômica usando a flag O_CREAT | O_EXCL na função open() do worker.

**Como o coordinator consegue ler o resultado?**

 coordinator abre o arquivo password_found.txt, lê a linha "worker_id:senha", separa o id do worker e a senha usando o caractere :, e utiliza a função md5_string() para calcular o hash da senha encontrada.

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|------|-------|------|------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | 0s | 0s | 0s | 0|
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | 0 | 0 | 0 | 

**O speedup foi linear? Por quê?**
 O speedup não foi linear. O tempo de execução foi muito baixo, mas pode se afirmar que speedup em cenarios para buscas pequenas não e eficiente pois sofre com o overhead que acaba sendo pior que o tempo de resolver o problema em si.
---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
Implementar o forc e exec, foi resolvido estudando e pesquisando.

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [x] Código compila sem erros
- [x] Todos os TODOs foram implementados
- [x] Testes passam no `./tests/simple_test.sh`
- [x] Relatório preenchido