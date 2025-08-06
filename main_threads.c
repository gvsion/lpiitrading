#include "trading_system.h"
#include <math.h>

// Estruturas para threads
typedef struct {
    TradingSystem* sistema;
    int trader_id;
    pthread_t thread;
    int ativa;
} ThreadTrader;

typedef struct {
    TradingSystem* sistema;
    pthread_t thread;
    int ativa;
} ThreadPriceUpdater;

typedef struct {
    TradingSystem* sistema;
    pthread_t thread;
    int ativa;
} ThreadExecutor;

typedef struct {
    TradingSystem* sistema;
    pthread_t thread;
    int ativa;
} ThreadArbitrageMonitor;

static ThreadTrader threads_traders[MAX_TRADERS];
static ThreadPriceUpdater thread_price_updater;
static ThreadExecutor thread_executor;
static ThreadArbitrageMonitor thread_arbitrage_monitor;

// Funções de utilidade
double gerar_preco_aleatorio(double min, double max) {
    return min + (rand() / (double)RAND_MAX) * (max - min);
}

int gerar_id_aleatorio() {
    return rand() % 10000;
}

void log_evento(const char* mensagem) {
    time_t agora = time(NULL);
    char* timestamp = ctime(&agora);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remover \n
    printf("[%s] %s\n", timestamp, mensagem);
}

void limpar_tela() {
    printf("\033[2J\033[H"); // ANSI escape sequence
}

// Função principal do sistema
TradingSystem* inicializar_sistema() {
    TradingSystem* sistema = (TradingSystem*)malloc(sizeof(TradingSystem));
    if (!sistema) {
        printf("Erro: Falha ao alocar memória para o sistema\n");
        return NULL;
    }
    
    // Inicializar variáveis do sistema
    sistema->num_acoes = 0;
    sistema->num_traders = 0;
    sistema->num_ordens = 0;
    sistema->sistema_ativo = 1;
    
    // Inicializar mutex e semáforo
    pthread_mutex_init(&sistema->mutex_geral, NULL);
    sem_init(&sistema->sem_ordens, 0, 0);
    
    // Inicializar componentes
    inicializar_acoes(sistema);
    inicializar_traders(sistema);
    inicializar_executor(sistema);
    
    log_evento("Sistema de trading inicializado com sucesso");
    return sistema;
}

void limpar_sistema(TradingSystem* sistema) {
    if (!sistema) return;
    
    printf("=== FINALIZANDO SISTEMA ===\n");
    
    // Parar todas as threads
    parar_todas_threads();
    
    // Aguardar threads terminarem
    aguardar_threads_terminarem();
    
    // Limpar estruturas globais
    limpar_estruturas_globais();
    
    // Limpar mutexes
    for (int i = 0; i < sistema->num_acoes; i++) {
        pthread_mutex_destroy(&sistema->acoes[i].mutex);
    }
    
    for (int i = 0; i < sistema->num_traders; i++) {
        pthread_mutex_destroy(&sistema->traders[i].mutex);
    }
    
    pthread_mutex_destroy(&sistema->executor.mutex);
    pthread_mutex_destroy(&sistema->mutex_geral);
    sem_destroy(&sistema->sem_ordens);
    
    free(sistema);
    printf("✓ Sistema de trading finalizado\n");
}

// Funções de threads movidas para threads_sistema.c

// Função para iniciar todas as threads
void iniciar_threads(TradingSystem* sistema) {
    printf("=== INICIANDO THREADS ===\n");
    
    // Definir sistema global
    sistema_global = sistema;
    
    // Inicializar estruturas globais
    inicializar_estruturas_globais();
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Inicializar perfis de trader
    inicializar_perfis_trader();
    
    // Criar threads traders
    for (int i = 0; i < MAX_TRADERS; i++) {
        int perfil_id = i % 3; // Distribuir perfis entre traders
        if (!criar_thread_trader(i, perfil_id)) {
            printf("✗ Erro ao criar thread trader %d\n", i);
        }
    }
    
    // Criar thread executor
    if (!criar_thread_executor()) {
        printf("✗ Erro ao criar thread executor\n");
    }
    
    // Criar thread price updater
    if (!criar_thread_price_updater()) {
        printf("✗ Erro ao criar thread price updater\n");
    }
    
    // Criar thread arbitrage monitor
    if (!criar_thread_arbitrage_monitor()) {
        printf("✗ Erro ao criar thread arbitrage monitor\n");
    }
    
    printf("=== TODAS AS THREADS INICIADAS ===\n");
}

// Função para exibir estatísticas em tempo real
void exibir_estatisticas_tempo_real(TradingSystem* sistema) {
    static int contador = 0;
    contador++;
    
    if (contador % 10 == 0) { // A cada 10 iterações
        limpar_tela();
        printf("=== SISTEMA DE TRADING - VERSÃO THREADS ===\n");
        printf("Tempo de execução: %d segundos\n", contador * 2);
        printf("Sistema ativo: %s\n", sistema->sistema_ativo ? "SIM" : "NÃO");
        printf("\n");
        
        imprimir_estado_acoes(sistema);
        imprimir_estado_traders(sistema);
        imprimir_estado_executor(sistema);
        imprimir_oportunidades_arbitragem();
        imprimir_alertas();
        
        calcular_estatisticas_execucao(sistema);
        calcular_estatisticas_arbitragem(sistema);
    }
}

int main() {
    printf("=== SISTEMA DE TRADING - VERSÃO THREADS ===\n");
    printf("Iniciando sistema...\n\n");
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Inicializar sistema
    TradingSystem* sistema = inicializar_sistema();
    if (!sistema) {
        printf("Erro: Falha ao inicializar sistema\n");
        return 1;
    }
    
    // Iniciar threads
    iniciar_threads(sistema);
    
    printf("Sistema iniciado com sucesso!\n");
    printf("Pressione Ctrl+C para parar o sistema\n\n");
    
    // Loop principal
    int tempo_execucao = 0;
    while (sistema->sistema_ativo && tempo_execucao < 300) { // Executar por 5 minutos
        exibir_estatisticas_tempo_real(sistema);
        sleep(2);
        tempo_execucao += 2;
    }
    
    // Parar sistema
    printf("\nParando sistema...\n");
    sistema->sistema_ativo = 0;
    
    // Aguardar threads terminarem
    for (int i = 0; i < sistema->num_traders; i++) {
        if (threads_traders[i].ativa) {
            pthread_join(threads_traders[i].thread, NULL);
        }
    }
    
    if (thread_price_updater.ativa) {
        pthread_join(thread_price_updater.thread, NULL);
    }
    
    if (thread_executor.ativa) {
        pthread_join(thread_executor.thread, NULL);
    }
    
    if (thread_arbitrage_monitor.ativa) {
        pthread_join(thread_arbitrage_monitor.thread, NULL);
    }
    
    // Exibir estatísticas finais
    printf("\n=== ESTATÍSTICAS FINAIS ===\n");
    imprimir_estado_acoes(sistema);
    imprimir_estado_traders(sistema);
    imprimir_estado_executor(sistema);
    imprimir_ordens(sistema);
    imprimir_oportunidades_arbitragem();
    imprimir_alertas();
    
    // Limpar sistema
    limpar_sistema(sistema);
    
    printf("Sistema finalizado com sucesso!\n");
    return 0;
} 