#ifndef TRADING_SYSTEM_H
#define TRADING_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Constantes do sistema
#define MAX_ACOES 10
#define MAX_TRADERS 6
#define MAX_ORDENS 100
#define MAX_NOME 50
#define MAX_STRATEGY 20

// Estruturas de dados
typedef struct {
    char nome[MAX_NOME];
    double preco_atual;
    double preco_anterior;
    double variacao;
    int volume_negociado;
    pthread_mutex_t mutex;
} Acao;

typedef struct {
    int id;
    char nome[MAX_NOME];
    double saldo;
    int acoes_possuidas[MAX_ACOES];
    pthread_mutex_t mutex;
} Trader;

typedef struct {
    int id;
    int trader_id;
    int acao_id;
    char tipo; // 'C' para compra, 'V' para venda
    double preco;
    int quantidade;
    time_t timestamp;
    int status; // 0: pendente, 1: executada, 2: cancelada
} Ordem;

typedef struct {
    int id;
    char nome[MAX_NOME];
    double saldo_inicial;
    double saldo_atual;
    int total_ordens;
    int ordens_executadas;
    int ordens_canceladas;
    pthread_mutex_t mutex;
} Executor;

typedef struct {
    Acao acoes[MAX_ACOES];
    Trader traders[MAX_TRADERS];
    Ordem ordens[MAX_ORDENS];
    Executor executor;
    int num_acoes;
    int num_traders;
    int num_ordens;
    pthread_mutex_t mutex_geral;
    sem_t sem_ordens;
    int sistema_ativo;
} TradingSystem;

// Funções do sistema
TradingSystem* inicializar_sistema();
void limpar_sistema(TradingSystem* sistema);

// Funções de ações
void inicializar_acoes(TradingSystem* sistema);
void atualizar_preco_acao(TradingSystem* sistema, int acao_id, double novo_preco);
void imprimir_estado_acoes(TradingSystem* sistema);

// Funções de traders
void inicializar_traders(TradingSystem* sistema);
void executar_estrategia_trader(TradingSystem* sistema, int trader_id);
void imprimir_estado_traders(TradingSystem* sistema);

// Funções de ordens
int criar_ordem(TradingSystem* sistema, int trader_id, int acao_id, char tipo, double preco, int quantidade);
void processar_ordem(TradingSystem* sistema, int ordem_id);
void cancelar_ordem(TradingSystem* sistema, int ordem_id);
void imprimir_ordens(TradingSystem* sistema);

// Funções do executor
void inicializar_executor(TradingSystem* sistema);
void executar_ordens_pendentes(TradingSystem* sistema);
void imprimir_estado_executor(TradingSystem* sistema);

// Funções de monitoramento
void monitorar_arbitragem(TradingSystem* sistema);
void detectar_arbitragem(TradingSystem* sistema);
void registrar_oportunidade_arbitragem(int acao1_id, int acao2_id, double diferenca, double percentual);
void verificar_condicoes_mercado(TradingSystem* sistema);
void criar_alerta(const char* tipo, const char* descricao, double valor, int prioridade);
void imprimir_oportunidades_arbitragem();
void imprimir_alertas();
void calcular_estatisticas_arbitragem(TradingSystem* sistema);
void simular_evento_mercado(TradingSystem* sistema);
void detectar_padroes_preco(TradingSystem* sistema);
void calcular_estatisticas_execucao(TradingSystem* sistema);

// Funções utilitárias para dados financeiros
Ordem gerar_ordem_aleatoria(TradingSystem* sistema);
void gerar_ordens_aleatorias(TradingSystem* sistema, int num_ordens);
double calcular_preco_oferta_demanda(TradingSystem* sistema, int acao_id);
void detectar_arbitragem_relacionadas(TradingSystem* sistema);
int validar_ordem(TradingSystem* sistema, Ordem* ordem);
void imprimir_ordem(Ordem* ordem, TradingSystem* sistema);
void testar_funcoes_utilitarias(TradingSystem* sistema);
void inicializar_dados_mercado();
void imprimir_estatisticas_mercado(TradingSystem* sistema);

// Funções de utilidade
double gerar_preco_aleatorio(double min, double max);
int gerar_id_aleatorio();
void log_evento(const char* mensagem);
void limpar_tela();
void atualizar_todos_precos(TradingSystem* sistema);
void simular_noticia_mercado(TradingSystem* sistema);

// Estruturas para comunicação entre processos/threads
typedef struct {
    int tipo_mensagem; // 1: nova ordem, 2: atualizar preço, 3: executar ordem
    int id_origem;
    int id_destino;
    double valor;
    int quantidade;
    char dados_extras[100];
} Mensagem;

#endif // TRADING_SYSTEM_H 