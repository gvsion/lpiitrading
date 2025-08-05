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
    
    // Parar todas as threads
    sistema->sistema_ativo = 0;
    
    // Aguardar threads terminarem
    for (int i = 0; i < MAX_TRADERS; i++) {
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
    log_evento("Sistema de trading finalizado");
}

// Função da thread do trader
void* thread_trader_func(void* arg) {
    ThreadTrader* thread_data = (ThreadTrader*)arg;
    TradingSystem* sistema = thread_data->sistema;
    int trader_id = thread_data->trader_id;
    
    printf("Thread do trader %d iniciada\n", trader_id);
    
    while (sistema->sistema_ativo) {
        executar_estrategia_trader(sistema, trader_id);
        sleep(2 + (trader_id * 1)); // Cada trader opera em intervalos diferentes
    }
    
    printf("Thread do trader %d finalizada\n", trader_id);
    return NULL;
}

// Função da thread de atualização de preços
void* thread_price_updater_func(void* arg) {
    ThreadPriceUpdater* thread_data = (ThreadPriceUpdater*)arg;
    TradingSystem* sistema = thread_data->sistema;
    
    printf("Thread de atualização de preços iniciada\n");
    
    while (sistema->sistema_ativo) {
        atualizar_todos_precos(sistema);
        
        // Simular notícias ocasionalmente
        if (rand() % 100 < 10) { // 10% de chance
            simular_noticia_mercado(sistema);
        }
        
        sleep(3); // Atualizar preços a cada 3 segundos
    }
    
    printf("Thread de atualização de preços finalizada\n");
    return NULL;
}

// Função da thread do executor
void* thread_executor_func(void* arg) {
    ThreadExecutor* thread_data = (ThreadExecutor*)arg;
    TradingSystem* sistema = thread_data->sistema;
    
    printf("Thread do executor iniciada\n");
    
    while (sistema->sistema_ativo) {
        executar_ordens_pendentes(sistema);
        sleep(1); // Executar ordens a cada segundo
    }
    
    printf("Thread do executor finalizada\n");
    return NULL;
}

// Função da thread de monitoramento de arbitragem
void* thread_arbitrage_monitor_func(void* arg) {
    ThreadArbitrageMonitor* thread_data = (ThreadArbitrageMonitor*)arg;
    TradingSystem* sistema = thread_data->sistema;
    
    printf("Thread de monitoramento de arbitragem iniciada\n");
    
    while (sistema->sistema_ativo) {
        monitorar_arbitragem(sistema);
        detectar_padroes_preco(sistema);
        
        // Simular eventos de mercado ocasionalmente
        if (rand() % 100 < 5) { // 5% de chance
            simular_evento_mercado(sistema);
        }
        
        sleep(5); // Monitorar a cada 5 segundos
    }
    
    printf("Thread de monitoramento de arbitragem finalizada\n");
    return NULL;
}

// Função para iniciar todas as threads
void iniciar_threads(TradingSystem* sistema) {
    // Iniciar threads dos traders
    for (int i = 0; i < sistema->num_traders; i++) {
        threads_traders[i].sistema = sistema;
        threads_traders[i].trader_id = i;
        threads_traders[i].ativa = 1;
        
        if (pthread_create(&threads_traders[i].thread, NULL, thread_trader_func, &threads_traders[i]) != 0) {
            printf("Erro ao criar thread do trader %d\n", i);
            threads_traders[i].ativa = 0;
        }
    }
    
    // Iniciar thread de atualização de preços
    thread_price_updater.sistema = sistema;
    thread_price_updater.ativa = 1;
    if (pthread_create(&thread_price_updater.thread, NULL, thread_price_updater_func, &thread_price_updater) != 0) {
        printf("Erro ao criar thread de atualização de preços\n");
        thread_price_updater.ativa = 0;
    }
    
    // Iniciar thread do executor
    thread_executor.sistema = sistema;
    thread_executor.ativa = 1;
    if (pthread_create(&thread_executor.thread, NULL, thread_executor_func, &thread_executor) != 0) {
        printf("Erro ao criar thread do executor\n");
        thread_executor.ativa = 0;
    }
    
    // Iniciar thread de monitoramento de arbitragem
    thread_arbitrage_monitor.sistema = sistema;
    thread_arbitrage_monitor.ativa = 1;
    if (pthread_create(&thread_arbitrage_monitor.thread, NULL, thread_arbitrage_monitor_func, &thread_arbitrage_monitor) != 0) {
        printf("Erro ao criar thread de monitoramento de arbitragem\n");
        thread_arbitrage_monitor.ativa = 0;
    }
    
    log_evento("Todas as threads iniciadas");
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