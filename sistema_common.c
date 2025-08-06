#include "trading_system.h"
#include <math.h>
#include <unistd.h>

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
    
    // Inicializar dados do mercado
    inicializar_dados_mercado();
    
    // Inicializar componentes
    inicializar_acoes(sistema);
    inicializar_traders(sistema);
    inicializar_executor(sistema);
    
    log_evento("Sistema de trading inicializado com sucesso");
    return sistema;
}

void limpar_sistema(TradingSystem* sistema) {
    if (!sistema) return;
    
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