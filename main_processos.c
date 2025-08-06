#include "trading_system.h"
#include <math.h>
#include <fcntl.h>
#include <errno.h>

// Estruturas para processos
typedef struct {
    pid_t pid;
    int trader_id;
    int ativo;
} ProcessoTrader;

typedef struct {
    pid_t pid;
    int ativo;
} ProcessoPriceUpdater;

typedef struct {
    pid_t pid;
    int ativo;
} ProcessoExecutor;

typedef struct {
    pid_t pid;
    int ativo;
} ProcessoArbitrageMonitor;

static ProcessoTrader processos_traders[MAX_TRADERS];
static ProcessoPriceUpdater processo_price_updater;
static ProcessoExecutor processo_executor;
static ProcessoArbitrageMonitor processo_arbitrage_monitor;

// Funções dos processos
void processo_price_updater_func();
void processo_executor_func();
void processo_arbitrage_monitor_func();

// Variáveis globais para comunicação entre processos
static TradingSystem* sistema_compartilhado = NULL;

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

// Função para criar memória compartilhada
TradingSystem* criar_memoria_compartilhada() {
    // Criar segmento de memória compartilhada
    shm_id = shmget(IPC_PRIVATE, sizeof(TradingSystem), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Erro ao criar memória compartilhada");
        return NULL;
    }
    
    // Anexar memória compartilhada
    sistema_compartilhado = (TradingSystem*)shmat(shm_id, NULL, 0);
    if (sistema_compartilhado == (void*)-1) {
        perror("Erro ao anexar memória compartilhada");
        return NULL;
    }
    
    // Inicializar sistema na memória compartilhada
    memset(sistema_compartilhado, 0, sizeof(TradingSystem));
    sistema_compartilhado->num_acoes = 0;
    sistema_compartilhado->num_traders = 0;
    sistema_compartilhado->num_ordens = 0;
    sistema_compartilhado->sistema_ativo = 1;
    
    // Inicializar mutex e semáforo
    pthread_mutex_init(&sistema_compartilhado->mutex_geral, NULL);
    sem_init(&sistema_compartilhado->sem_ordens, 0, 0);
    
    // Inicializar componentes
    inicializar_acoes(sistema_compartilhado);
    inicializar_traders(sistema_compartilhado);
    inicializar_executor(sistema_compartilhado);
    
    log_evento("Memória compartilhada criada e inicializada");
    return sistema_compartilhado;
}

// Função para limpar memória compartilhada
void limpar_memoria_compartilhada() {
    if (sistema_compartilhado) {
        // Limpar mutexes
        for (int i = 0; i < sistema_compartilhado->num_acoes; i++) {
            pthread_mutex_destroy(&sistema_compartilhado->acoes[i].mutex);
        }
        
        for (int i = 0; i < sistema_compartilhado->num_traders; i++) {
            pthread_mutex_destroy(&sistema_compartilhado->traders[i].mutex);
        }
        
        pthread_mutex_destroy(&sistema_compartilhado->executor.mutex);
        pthread_mutex_destroy(&sistema_compartilhado->mutex_geral);
        sem_destroy(&sistema_compartilhado->sem_ordens);
        
        // Desanexar memória compartilhada
        shmdt(sistema_compartilhado);
        
        // Remover segmento de memória compartilhada
        shmctl(shm_id, IPC_RMID, NULL);
        
        log_evento("Memória compartilhada limpa");
    }
}

// Função do processo trader
void processo_trader(int trader_id) {
    // Usar a versão melhorada com perfil
    int perfil_id = trader_id % 3; // Distribuir perfis entre traders
    processo_trader_melhorado(trader_id, perfil_id);
}

// Função do processo price updater
void processo_price_updater_func() {
    // Usar a versão melhorada do price updater
    processo_price_updater_melhorado();
}

// Função do processo executor
void processo_executor_func() {
    // Usar a versão melhorada do executor
    processo_executor_melhorado();
}

// Função do processo arbitrage monitor
void processo_arbitrage_monitor_func() {
    printf("Processo de monitoramento de arbitragem iniciado (PID: %d)\n", getpid());
    
    // Anexar memória compartilhada
    TradingSystem* sistema = (TradingSystem*)shmat(shm_id, NULL, 0);
    if (sistema == (void*)-1) {
        perror("Erro ao anexar memória compartilhada no processo arbitrage monitor");
        exit(1);
    }
    
    while (sistema->sistema_ativo) {
        monitorar_arbitragem(sistema);
        detectar_padroes_preco(sistema);
        
        // Simular eventos de mercado ocasionalmente
        if (rand() % 100 < 5) { // 5% de chance
            simular_evento_mercado(sistema);
        }
        
        sleep(5); // Monitorar a cada 5 segundos
    }
    
    // Desanexar memória compartilhada
    shmdt(sistema);
    
    printf("Processo de monitoramento de arbitragem finalizado\n");
    exit(0);
}

// Função para iniciar todos os processos
void iniciar_processos() {
    printf("=== INICIANDO PROCESSOS COM PIPES ===\n");
    
    // Inicializar métricas de performance
    inicializar_metricas_performance();
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Iniciar medição de tempo de criação
    iniciar_medicao_criacao(1); // 1 = processos
    
    // Criar pipes do sistema
    int* descritores = criar_pipes_sistema();
    if (!descritores) {
        printf("Erro: Falha ao criar pipes do sistema\n");
        return;
    }
    
    // Criar memória compartilhada
    if (!criar_memoria_compartilhada()) {
        printf("Erro: Falha ao criar memória compartilhada\n");
        limpar_pipes_sistema();
        return;
    }
    
    // Inicializar estruturas de processos
    memset(processos_traders, 0, sizeof(processos_traders));
    memset(&processo_price_updater, 0, sizeof(processo_price_updater));
    memset(&processo_executor, 0, sizeof(processo_executor));
    memset(&processo_arbitrage_monitor, 0, sizeof(processo_arbitrage_monitor));
    
    // Iniciar processo Price Updater
    pid_t pid_price = fork();
    if (pid_price == 0) {
        // Processo filho - fechar descritores desnecessários
        close(descritores[0]); // Traders->Executor RD
        close(descritores[1]); // Traders->Executor WR
        close(descritores[2]); // Executor->PriceUpdater RD
        close(descritores[5]); // PriceUpdater->Arbitrage WR
        close(descritores[6]); // Arbitrage->Traders RD
        close(descritores[7]); // Arbitrage->Traders WR
        close(descritores[8]); // Control RD
        close(descritores[9]); // Control WR
        
        processo_price_updater_func();
        exit(0);
    } else if (pid_price > 0) {
        processo_price_updater.pid = pid_price;
        processo_price_updater.ativo = 1;
        printf("✓ Processo Price Updater iniciado (PID: %d)\n", pid_price);
    } else {
        perror("Erro ao criar processo Price Updater");
        limpar_pipes_sistema();
        return;
    }
    
    // Iniciar processo Executor
    pid_t pid_executor = fork();
    if (pid_executor == 0) {
        // Processo filho - fechar descritores desnecessários
        close(descritores[0]); // Traders->Executor RD
        close(descritores[3]); // Executor->PriceUpdater WR
        close(descritores[4]); // PriceUpdater->Arbitrage RD
        close(descritores[5]); // PriceUpdater->Arbitrage WR
        close(descritores[6]); // Arbitrage->Traders RD
        close(descritores[7]); // Arbitrage->Traders WR
        close(descritores[8]); // Control RD
        close(descritores[9]); // Control WR
        
        processo_executor_func();
        exit(0);
    } else if (pid_executor > 0) {
        processo_executor.pid = pid_executor;
        processo_executor.ativo = 1;
        printf("✓ Processo Executor iniciado (PID: %d)\n", pid_executor);
    } else {
        perror("Erro ao criar processo Executor");
        limpar_pipes_sistema();
        return;
    }
    
    // Iniciar processo Arbitrage Monitor
    pid_t pid_arbitrage = fork();
    if (pid_arbitrage == 0) {
        // Processo filho - fechar descritores desnecessários
        close(descritores[0]); // Traders->Executor RD
        close(descritores[1]); // Traders->Executor WR
        close(descritores[2]); // Executor->PriceUpdater RD
        close(descritores[3]); // Executor->PriceUpdater WR
        close(descritores[4]); // PriceUpdater->Arbitrage RD
        close(descritores[7]); // Arbitrage->Traders WR
        close(descritores[8]); // Control RD
        close(descritores[9]); // Control WR
        
        processo_arbitrage_monitor_func();
        exit(0);
    } else if (pid_arbitrage > 0) {
        processo_arbitrage_monitor.pid = pid_arbitrage;
        processo_arbitrage_monitor.ativo = 1;
        printf("✓ Processo Arbitrage Monitor iniciado (PID: %d)\n", pid_arbitrage);
    } else {
        perror("Erro ao criar processo Arbitrage Monitor");
        limpar_pipes_sistema();
        return;
    }
    
    // Iniciar processos Traders
    for (int i = 0; i < MAX_TRADERS; i++) {
        processos_traders[i].trader_id = i;
        processos_traders[i].ativo = 0;
        
        pid_t pid_trader = fork();
        if (pid_trader == 0) {
            // Processo filho - fechar descritores desnecessários
            close(descritores[2]); // Executor->PriceUpdater RD
            close(descritores[3]); // Executor->PriceUpdater WR
            close(descritores[4]); // PriceUpdater->Arbitrage RD
            close(descritores[5]); // PriceUpdater->Arbitrage WR
            close(descritores[6]); // Arbitrage->Traders RD
            close(descritores[8]); // Control RD
            close(descritores[9]); // Control WR
            
            processo_trader(i);
            exit(0);
        } else if (pid_trader > 0) {
            processos_traders[i].pid = pid_trader;
            processos_traders[i].ativo = 1;
            printf("✓ Processo Trader %d iniciado (PID: %d)\n", i, pid_trader);
        } else {
            perror("Erro ao criar processo Trader");
            limpar_pipes_sistema();
            return;
        }
    }
    
    // Finalizar medição de tempo de criação
    finalizar_medicao_criacao(1); // 1 = processos
    
    printf("=== TODOS OS PROCESSOS INICIADOS COM PIPES ===\n\n");
    log_evento("Todos os processos iniciados com pipes");
}

// Função para parar todos os processos
void parar_processos() {
    printf("=== PARANDO PROCESSOS E LIMPANDO PIPES ===\n");
    
    // Parar processo de arbitragem
    if (processo_arbitrage_monitor.ativo) {
        kill(processo_arbitrage_monitor.pid, SIGTERM);
        waitpid(processo_arbitrage_monitor.pid, NULL, 0);
        processo_arbitrage_monitor.ativo = 0;
        printf("✓ Processo Arbitrage Monitor parado\n");
    }
    
    // Parar processo executor
    if (processo_executor.ativo) {
        kill(processo_executor.pid, SIGTERM);
        waitpid(processo_executor.pid, NULL, 0);
        processo_executor.ativo = 0;
        printf("✓ Processo Executor parado\n");
    }
    
    // Parar processo price updater
    if (processo_price_updater.ativo) {
        kill(processo_price_updater.pid, SIGTERM);
        waitpid(processo_price_updater.pid, NULL, 0);
        processo_price_updater.ativo = 0;
        printf("✓ Processo Price Updater parado\n");
    }
    
    // Parar processos dos traders
    for (int i = 0; i < MAX_TRADERS; i++) {
        if (processos_traders[i].ativo) {
            kill(processos_traders[i].pid, SIGTERM);
            waitpid(processos_traders[i].pid, NULL, 0);
            processos_traders[i].ativo = 0;
            printf("✓ Processo Trader %d parado\n", i);
        }
    }
    
    // Calcular métricas finais
    calcular_metricas_mercado(sistema_compartilhado);
    calcular_throughput(1, 30.0); // 1 = processos, 30 segundos estimados
    
    // Exibir métricas de performance
    exibir_metricas_performance(1); // 1 = processos
    exibir_metricas_mercado();
    
    // Limpar pipes do sistema
    if (pipes_estao_ativos()) {
        limpar_pipes_sistema();
    }
    
    printf("=== TODOS OS PROCESSOS PARADOS E PIPES LIMPOS ===\n\n");
    log_evento("Todos os processos parados e pipes limpos");
}

// Função para exibir estatísticas em tempo real
void exibir_estatisticas_tempo_real() {
    static int contador = 0;
    contador++;
    
    if (contador % 10 == 0) { // A cada 10 iterações
        limpar_tela();
        printf("=== SISTEMA DE TRADING - VERSÃO PROCESSOS ===\n");
        printf("Tempo de execução: %d segundos\n", contador * 2);
        printf("Sistema ativo: %s\n", sistema_compartilhado->sistema_ativo ? "SIM" : "NÃO");
        printf("\n");
        
        imprimir_estado_acoes(sistema_compartilhado);
        imprimir_estado_traders(sistema_compartilhado);
        imprimir_estado_executor(sistema_compartilhado);
        imprimir_oportunidades_arbitragem();
        imprimir_alertas();
        
        calcular_estatisticas_execucao(sistema_compartilhado);
        calcular_estatisticas_arbitragem(sistema_compartilhado);
    }
}

// Handler para SIGINT (Ctrl+C)
void signal_handler(int sig) {
    (void)sig; // Evitar warning de parâmetro não utilizado
    printf("\nRecebido sinal de interrupção. Parando sistema...\n");
    if (sistema_compartilhado) {
        sistema_compartilhado->sistema_ativo = 0;
    }
}

int main() {
    printf("=== SISTEMA DE TRADING - VERSÃO PROCESSOS ===\n");
    printf("Iniciando sistema...\n\n");
    
    // Configurar handler para SIGINT
    signal(SIGINT, signal_handler);
    
    // Inicializar métricas de performance
    inicializar_metricas_performance();
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Inicializar perfis de trader
    inicializar_perfis_trader();
    
    // Criar memória compartilhada
    if (!criar_memoria_compartilhada()) {
        printf("Erro: Falha ao criar memória compartilhada\n");
        return 1;
    }
    
    // Iniciar processos
    iniciar_processos();
    
    printf("Sistema iniciado com sucesso!\n");
    printf("Pressione Ctrl+C para parar o sistema\n\n");
    
    // Loop principal do processo pai
    int tempo_execucao = 0;
    while (sistema_compartilhado->sistema_ativo && tempo_execucao < 300) { // Executar por 5 minutos
        exibir_estatisticas_tempo_real();
        sleep(2);
        tempo_execucao += 2;
    }
    
    // Parar sistema
    printf("\nParando sistema...\n");
    sistema_compartilhado->sistema_ativo = 0;
    
    // Aguardar processos terminarem
    parar_processos();
    
    // Exibir estatísticas finais
    printf("\n=== ESTATÍSTICAS FINAIS ===\n");
    imprimir_estado_acoes(sistema_compartilhado);
    imprimir_estado_traders(sistema_compartilhado);
    imprimir_estado_executor(sistema_compartilhado);
    imprimir_ordens(sistema_compartilhado);
    imprimir_oportunidades_arbitragem();
    imprimir_alertas();
    
    // Limpar memória compartilhada
    limpar_memoria_compartilhada();
    
    printf("Sistema finalizado com sucesso!\n");
    return 0;
} 