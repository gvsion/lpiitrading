#include "trading_system.h"
#include <pthread.h>
#include <unistd.h>

// Estruturas para demonstrar race conditions
typedef struct {
    int id;
    double preco;
    int quantidade;
    char tipo;
    int corrompido; // Flag para indicar dados corrompidos
} OrdemRace;

typedef struct {
    double preco;
    int volume;
    int operacoes;
    int corrompido; // Flag para indicar dados corrompidos
} AcaoRace;

// Dados globais SEM sincronização (deliberadamente)
static OrdemRace ordens_race[100]; // Array compartilhado sem proteção
static AcaoRace acoes_race[10];    // Ações compartilhadas sem proteção
static int contador_global = 0;     // Contador global sem proteção
static int indice_ordem = 0;        // Índice compartilhado sem proteção

// Flags para controlar o demo
static int demo_ativo = 1;
static int detectar_corrupcao = 1;

// Estruturas para threads
typedef struct {
    int thread_id;
    int num_iteracoes;
    int delay_ms;
} ThreadParams;

// Função para detectar inconsistências nos dados
void detectar_inconsistencias() {
    printf("\n=== DETECÇÃO DE INCONSISTÊNCIAS ===\n");
    
    int ordens_corrompidas = 0;
    int acoes_corrompidas = 0;
    int problemas_contador = 0;
    
    // Verificar ordens corrompidas
    for (int i = 0; i < 100; i++) {
        if (ordens_race[i].corrompido) {
            ordens_corrompidas++;
            printf("❌ Ordem %d corrompida: ID=%d, Preço=%.2f, Qtd=%d, Tipo=%c\n", 
                   i, ordens_race[i].id, ordens_race[i].preco, 
                   ordens_race[i].quantidade, ordens_race[i].tipo);
        }
    }
    
    // Verificar ações corrompidas
    for (int i = 0; i < 10; i++) {
        if (acoes_race[i].corrompido) {
            acoes_corrompidas++;
            printf("❌ Ação %d corrompida: Preço=%.2f, Volume=%d, Operações=%d\n", 
                   i, acoes_race[i].preco, acoes_race[i].volume, acoes_race[i].operacoes);
        }
    }
    
    // Verificar contador global
    if (contador_global < 0) {
        problemas_contador++;
        printf("❌ Contador global inválido: %d\n", contador_global);
    }
    
    // Verificar índice de ordem
    if (indice_ordem < 0 || indice_ordem >= 100) {
        problemas_contador++;
        printf("❌ Índice de ordem inválido: %d\n", indice_ordem);
    }
    
    printf("\n=== RESUMO DE PROBLEMAS ===\n");
    printf("Ordens corrompidas: %d\n", ordens_corrompidas);
    printf("Ações corrompidas: %d\n", acoes_corrompidas);
    printf("Problemas de contador: %d\n", problemas_contador);
    printf("Total de problemas: %d\n", ordens_corrompidas + acoes_corrompidas + problemas_contador);
    
    if (ordens_corrompidas + acoes_corrompidas + problemas_contador > 0) {
        printf("🚨 RACE CONDITIONS DETECTADAS! 🚨\n");
    } else {
        printf("✅ Nenhuma inconsistência detectada (pode ser sorte)\n");
    }
}

// Função para inicializar dados de teste
void inicializar_dados_race_conditions() {
    printf("=== INICIALIZANDO DADOS PARA DEMO DE RACE CONDITIONS ===\n");
    
    // Inicializar ordens
    for (int i = 0; i < 100; i++) {
        ordens_race[i].id = i;
        ordens_race[i].preco = 0.0;
        ordens_race[i].quantidade = 0;
        ordens_race[i].tipo = 'N';
        ordens_race[i].corrompido = 0;
    }
    
    // Inicializar ações
    for (int i = 0; i < 10; i++) {
        acoes_race[i].preco = 10.0 + i;
        acoes_race[i].volume = 1000;
        acoes_race[i].operacoes = 0;
        acoes_race[i].corrompido = 0;
    }
    
    // Inicializar contadores
    contador_global = 0;
    indice_ordem = 0;
    
    printf("✓ Dados inicializados (SEM sincronização)\n");
    printf("⚠️  AVISO: Este demo irá gerar race conditions deliberadamente!\n");
}

// Thread que escreve na mesma posição do array (RACE CONDITION 1)
void* thread_trader_race(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int thread_id = params->thread_id;
    int num_iteracoes = params->num_iteracoes;
    int delay_ms = params->delay_ms;
    
    printf("🚀 Thread Trader %d iniciada (iterações: %d, delay: %dms)\n", 
           thread_id, num_iteracoes, delay_ms);
    
    for (int i = 0; i < num_iteracoes && demo_ativo; i++) {
        // DELIBERADAMENTE escrever na mesma posição sem sincronização
        int posicao = indice_ordem % 100; // Race condition no índice
        
        // Log da operação de escrita
        log_operation(thread_id, "WRITE_ARRAY", "ORDEM", posicao, 
                     ordens_race[posicao].id, thread_id * 1000 + i, 
                     "Escrita na posição do array");
        
        // Simular operação não-atômica
        ordens_race[posicao].id = thread_id * 1000 + i;
        usleep(delay_ms * 1000); // Delay estratégico para tornar race condition visível
        
        // Log da operação de preço
        log_operation(thread_id, "WRITE_PRECO", "PRECO", posicao, 
                     ordens_race[posicao].preco, 10.0 + (thread_id * 0.1) + (i * 0.01), 
                     "Atualização de preço");
        
        ordens_race[posicao].preco = 10.0 + (thread_id * 0.1) + (i * 0.01);
        usleep(delay_ms * 1000);
        
        // Log da operação de quantidade
        log_operation(thread_id, "WRITE_QUANTIDADE", "VOLUME", posicao, 
                     ordens_race[posicao].quantidade, 100 + thread_id + i, 
                     "Atualização de quantidade");
        
        ordens_race[posicao].quantidade = 100 + thread_id + i;
        usleep(delay_ms * 1000);
        
        // Log da operação de tipo
        log_operation(thread_id, "WRITE_TIPO", "TIPO", posicao, 
                     ordens_race[posicao].tipo, (thread_id % 2 == 0) ? 'C' : 'V', 
                     "Atualização de tipo");
        
        ordens_race[posicao].tipo = (thread_id % 2 == 0) ? 'C' : 'V';
        usleep(delay_ms * 1000);
        
        // Marcar como potencialmente corrompido
        ordens_race[posicao].corrompido = 1;
        
        // Log da operação de índice
        log_operation(thread_id, "WRITE_INDICE", "CONTADOR", 0, 
                     indice_ordem, indice_ordem + 1, 
                     "Incremento de índice");
        
        // Incrementar índice de forma não-atômica
        indice_ordem++;
        usleep(delay_ms * 1000);
        
        // Detectar race condition em tempo real
        detectar_race_condition_tempo_real(thread_id, "WRITE_ARRAY", "ORDEM", posicao, 
                                         ordens_race[posicao].id, thread_id * 1000 + i);
        
        printf("Thread %d: Escreveu na posição %d (ID: %d, Preço: %.2f, Qtd: %d, Tipo: %c)\n",
               thread_id, posicao, ordens_race[posicao].id, ordens_race[posicao].preco,
               ordens_race[posicao].quantidade, ordens_race[posicao].tipo);
    }
    
    printf("✅ Thread Trader %d finalizada\n", thread_id);
    free(params);
    return NULL;
}

// Thread que modifica preços simultaneamente (RACE CONDITION 2)
void* thread_executor_race(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int thread_id = params->thread_id;
    int num_iteracoes = params->num_iteracoes;
    int delay_ms = params->delay_ms;
    
    printf("🚀 Thread Executor %d iniciada (iterações: %d, delay: %dms)\n", 
           thread_id, num_iteracoes, delay_ms);
    
    for (int i = 0; i < num_iteracoes && demo_ativo; i++) {
        // DELIBERADAMENTE modificar a mesma ação sem sincronização
        int acao_id = i % 10;
        
        // Log da leitura de preço
        log_operation(thread_id, "READ_PRECO", "PRECO", acao_id, 
                     acoes_race[acao_id].preco, acoes_race[acao_id].preco, 
                     "Leitura de preço para modificação");
        
        // Operação não-atômica: ler, modificar, escrever
        double preco_atual = acoes_race[acao_id].preco;
        usleep(delay_ms * 1000); // Delay estratégico
        
        preco_atual += 0.1; // Modificar preço
        usleep(delay_ms * 1000);
        
        // Log da escrita de preço
        log_operation(thread_id, "WRITE_PRECO", "PRECO", acao_id, 
                     acoes_race[acao_id].preco, preco_atual, 
                     "Escrita de preço modificado");
        
        acoes_race[acao_id].preco = preco_atual; // Escrever de volta
        usleep(delay_ms * 1000);
        
        // Log da leitura de volume
        log_operation(thread_id, "READ_VOLUME", "VOLUME", acao_id, 
                     acoes_race[acao_id].volume, acoes_race[acao_id].volume, 
                     "Leitura de volume para modificação");
        
        // Modificar volume de forma não-atômica
        int volume_atual = acoes_race[acao_id].volume;
        usleep(delay_ms * 1000);
        
        volume_atual += 10;
        usleep(delay_ms * 1000);
        
        // Log da escrita de volume
        log_operation(thread_id, "WRITE_VOLUME", "VOLUME", acao_id, 
                     acoes_race[acao_id].volume, volume_atual, 
                     "Escrita de volume modificado");
        
        acoes_race[acao_id].volume = volume_atual;
        usleep(delay_ms * 1000);
        
        // Log da operação de incremento
        log_operation(thread_id, "WRITE_OPERACOES", "CONTADOR", acao_id, 
                     acoes_race[acao_id].operacoes, acoes_race[acao_id].operacoes + 1, 
                     "Incremento de operações");
        
        // Incrementar operações de forma não-atômica
        acoes_race[acao_id].operacoes++;
        usleep(delay_ms * 1000);
        
        // Marcar como potencialmente corrompido
        acoes_race[acao_id].corrompido = 1;
        
        // Detectar race condition em tempo real
        detectar_race_condition_tempo_real(thread_id, "WRITE_PRECO", "PRECO", acao_id, 
                                         acoes_race[acao_id].preco, preco_atual);
        
        printf("Thread %d: Modificou ação %d (Preço: %.2f, Volume: %d, Operações: %d)\n",
               thread_id, acao_id, acoes_race[acao_id].preco, 
               acoes_race[acao_id].volume, acoes_race[acao_id].operacoes);
    }
    
    printf("✅ Thread Executor %d finalizada\n", thread_id);
    free(params);
    return NULL;
}

// Thread que incrementa contador global de forma não-atômica (RACE CONDITION 3)
void* thread_contador_race(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int thread_id = params->thread_id;
    int num_iteracoes = params->num_iteracoes;
    int delay_ms = params->delay_ms;
    
    printf("🚀 Thread Contador %d iniciada (iterações: %d, delay: %dms)\n", 
           thread_id, num_iteracoes, delay_ms);
    
    for (int i = 0; i < num_iteracoes && demo_ativo; i++) {
        // Log da leitura do contador
        log_operation(thread_id, "READ_CONTADOR", "CONTADOR", 0, 
                     contador_global, contador_global, 
                     "Leitura do contador global");
        
        // DELIBERADAMENTE incrementar contador de forma não-atômica
        int valor_atual = contador_global;
        usleep(delay_ms * 1000); // Delay estratégico
        
        valor_atual++; // Incrementar
        usleep(delay_ms * 1000);
        
        // Log da escrita do contador
        log_operation(thread_id, "WRITE_CONTADOR", "CONTADOR", 0, 
                     contador_global, valor_atual, 
                     "Escrita do contador global incrementado");
        
        contador_global = valor_atual; // Escrever de volta
        usleep(delay_ms * 1000);
        
        // Detectar race condition em tempo real
        detectar_race_condition_tempo_real(thread_id, "WRITE_CONTADOR", "CONTADOR", 0, 
                                         contador_global, valor_atual);
        
        printf("Thread %d: Incrementou contador para %d\n", thread_id, contador_global);
    }
    
    printf("✅ Thread Contador %d finalizada\n", thread_id);
    free(params);
    return NULL;
}

// Função para executar demo de race conditions
void executar_demo_race_conditions() {
    printf("\n=== DEMO DE RACE CONDITIONS ===\n");
    printf("⚠️  AVISO: Este demo irá gerar inconsistências deliberadamente!\n\n");
    
    // Inicializar sistema de logging
    inicializar_race_condition_logger();
    
    // Inicializar dados
    inicializar_dados_race_conditions();
    
    // Configurar threads
    pthread_t threads[6];
    ThreadParams* params[6];
    
    // Criar threads traders (escrevem na mesma posição)
    for (int i = 0; i < 3; i++) {
        params[i] = malloc(sizeof(ThreadParams));
        params[i]->thread_id = i;
        params[i]->num_iteracoes = 10;
        params[i]->delay_ms = 50; // Delay estratégico
        
        pthread_create(&threads[i], NULL, thread_trader_race, params[i]);
    }
    
    // Criar threads executoras (modificam preços simultaneamente)
    for (int i = 3; i < 5; i++) {
        params[i] = malloc(sizeof(ThreadParams));
        params[i]->thread_id = i;
        params[i]->num_iteracoes = 15;
        params[i]->delay_ms = 30; // Delay estratégico
        
        pthread_create(&threads[i], NULL, thread_executor_race, params[i]);
    }
    
    // Criar thread contador (incrementa contador global)
    params[5] = malloc(sizeof(ThreadParams));
    params[5]->thread_id = 5;
    params[5]->num_iteracoes = 20;
    params[5]->delay_ms = 20; // Delay estratégico
    
    pthread_create(&threads[5], NULL, thread_contador_race, params[5]);
    
    // Aguardar threads terminarem
    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Detectar inconsistências
    if (detectar_corrupcao) {
        detectar_inconsistencias();
    }
    
    // Gerar relatórios de logging
    gerar_relatorio_diferencas_execucoes();
    analisar_padroes_race_conditions();
    
    // Finalizar logging
    finalizar_race_condition_logger();
    
    printf("\n=== DEMO FINALIZADO ===\n");
    printf("Contador global final: %d\n", contador_global);
    printf("Índice de ordem final: %d\n", indice_ordem);
}

// Função para executar múltiplas vezes para observar comportamentos diferentes
void executar_multiplas_vezes(int num_execucoes) {
    printf("\n=== EXECUTANDO DEMO %d VEZES ===\n", num_execucoes);
    printf("Cada execução pode ter resultados diferentes devido às race conditions!\n\n");
    
    for (int exec = 1; exec <= num_execucoes; exec++) {
        printf("\n--- EXECUÇÃO %d/%d ---\n", exec, num_execucoes);
        
        // Resetar dados
        inicializar_dados_race_conditions();
        
        // Executar demo
        executar_demo_race_conditions();
        
        // Pequena pausa entre execuções
        if (exec < num_execucoes) {
            printf("Aguardando 2 segundos antes da próxima execução...\n");
            sleep(2);
        }
    }
    
    printf("\n=== TODAS AS EXECUÇÕES FINALIZADAS ===\n");
    printf("Observe como os resultados variam entre execuções!\n");
}

// Função para demonstrar diferentes tipos de race conditions
void demonstrar_tipos_race_conditions() {
    printf("\n=== TIPOS DE RACE CONDITIONS DEMONSTRADOS ===\n");
    
    printf("1. RACE CONDITION EM ARRAY:\n");
    printf("   - Múltiplas threads escrevem na mesma posição\n");
    printf("   - Dados podem ser sobrescritos ou corrompidos\n");
    printf("   - Índice compartilhado sem proteção\n\n");
    
    printf("2. RACE CONDITION EM PREÇOS:\n");
    printf("   - Operações read-modify-write não-atômicas\n");
    printf("   - Múltiplas threads modificam o mesmo preço\n");
    printf("   - Valores podem ser perdidos ou incorretos\n\n");
    
    printf("3. RACE CONDITION EM CONTADORES:\n");
    printf("   - Incremento não-atômico de contador global\n");
    printf("   - Múltiplas threads incrementam simultaneamente\n");
    printf("   - Alguns incrementos podem ser perdidos\n\n");
    
    printf("4. DELAYS ESTRATÉGICOS:\n");
    printf("   - usleep() para tornar race conditions mais visíveis\n");
    printf("   - Aumenta probabilidade de interleaving problemático\n");
    printf("   - Facilita observação de inconsistências\n\n");
}

// Função principal do demo
void demo_race_conditions() {
    printf("=== DEMONSTRAÇÃO DE RACE CONDITIONS ===\n");
    printf("⚠️  AVISO: Este código gera problemas deliberadamente!\n");
    printf("⚠️  NÃO use em produção - apenas para demonstração!\n\n");
    
    printf("Escolha uma opção:\n");
    printf("1. Executar demo uma vez\n");
    printf("2. Executar demo 10 vezes com logging detalhado\n");
    printf("Digite sua escolha (1 ou 2): ");
    
    int escolha;
    scanf("%d", &escolha);
    
    // Configurar parâmetros
    detectar_corrupcao = 1;
    demo_ativo = 1;
    
    if (escolha == 2) {
        // Executar múltiplas vezes com logging detalhado
        executar_multiplas_vezes_com_logging(10);
        
        // Comparar arquivos de log
        comparar_arquivos_log(10);
        
        printf("\n=== EXECUÇÕES MÚLTIPLAS CONCLUÍDAS ===\n");
        printf("Logs salvos em arquivos: race_condition_log_1.txt até race_condition_log_10.txt\n");
        printf("Verifique os logs para análise detalhada das race conditions!\n");
    } else {
        // Explicar tipos de race conditions
        demonstrar_tipos_race_conditions();
        
        // Executar demo uma vez
        executar_demo_race_conditions();
        
        printf("\n=== DEMO CONCLUÍDO ===\n");
        printf("✅ Race conditions demonstradas com sucesso!\n");
        printf("📚 Use este conhecimento para implementar sincronização adequada.\n");
    }
} 