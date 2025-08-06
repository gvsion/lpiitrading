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
#include <poll.h>

// Constantes do sistema
#define MAX_ACOES 13
#define MAX_TRADERS 6
#define MAX_ORDENS 100
#define MAX_NOME 50
#define MAX_STRATEGY 20

// Constantes para perfis de trader
#define PERFIL_CONSERVADOR 0
#define PERFIL_AGRESSIVO 1
#define PERFIL_DAY_TRADER 2

// Constantes para limites de processo
#define MAX_ORDENS_POR_TRADER 50
#define TEMPO_LIMITE_PROCESSO 300 // 5 minutos
#define INTERVALO_MIN_ORDENS 1
#define INTERVALO_MAX_ORDENS 3

// Constantes para executor
#define TEMPO_PROCESSAMENTO_MIN 50  // 50ms
#define TEMPO_PROCESSAMENTO_MAX 200 // 200ms
#define TIMEOUT_PIPE_READ 100       // 100ms timeout para leitura de pipes
#define MAX_VOLATILIDADE_ACEITA 0.15 // 15% de volatilidade máxima
#define MAX_VOLUME_ACEITO 10000     // Volume máximo aceito por ordem
#define MIN_VOLUME_ACEITO 10        // Volume mínimo aceito por ordem

// Constantes para price updater
#define MAX_VARIACAO_PRECO 0.20     // 20% de variação máxima
#define MIN_PRECO_ACAO 0.50         // Preço mínimo de ação
#define MAX_PRECO_ACAO 1000.0       // Preço máximo de ação
#define PESO_ULTIMA_TRANSACAO 0.6   // Peso da última transação (60%)
#define PESO_PRECO_ATUAL 0.4        // Peso do preço atual (40%)
#define ARQUIVO_HISTORICO "historico_precos.txt" // Arquivo para salvar histórico

// Constantes para threads
#define MAX_FILA_ORDENS 1000        // Tamanho máximo da fila de ordens
#define TIMEOUT_THREAD_JOIN 5000    // 5 segundos timeout para join
#define MAX_TENTATIVAS_THREAD 3     // Máximo de tentativas para criar thread
#define MAX_OPORTUNIDADES 50        // Máximo de oportunidades de arbitragem
#define MAX_LOG_ENTRIES 10000       // Máximo de entradas de log

// Estruturas globais para threads
typedef struct {
    int sistema_ativo;
    int mercado_aberto;
    time_t inicio_sessao;
    pthread_mutex_t mutex;
} EstadoMercado;

// Estruturas de dados
typedef struct {
    char nome[MAX_NOME];
    char setor[MAX_NOME];
    double preco_atual;
    double preco_anterior;
    double preco_maximo;
    double preco_minimo;
    double variacao;
    double volatilidade;
    int volume_negociado;
    int volume_diario;
    int volume_total;
    int num_operacoes;
    double variacao_diaria;
    double variacao_semanal;
    double variacao_mensal;
    double historico_precos[30];
    int indice_historico;
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

// Estrutura para fila de ordens (após definição de Ordem)
typedef struct {
    Ordem ordens[MAX_FILA_ORDENS];
    int inicio;
    int fim;
    int tamanho;
    pthread_mutex_t mutex;
    pthread_cond_t cond_nao_vazia;
    pthread_cond_t cond_nao_cheia;
} FilaOrdens;

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

// Estrutura para perfil de trader
typedef struct {
    int perfil_id;
    char nome[MAX_NOME];
    int intervalo_min_ordens; // segundos
    int intervalo_max_ordens; // segundos
    int max_ordens_por_sessao;
    int tempo_limite_sessao; // segundos
    double agressividade; // 0.0 a 1.0
    double volume_medio; // quantidade média de ações
    int acoes_preferidas[MAX_ACOES];
    int num_acoes_preferidas;
} PerfilTrader;

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

// Funções de mercado
void inicializar_dados_mercado();
void inicializar_acoes_mercado(TradingSystem* sistema);
int mercado_esta_aberto();
char* obter_horario_abertura();
char* obter_horario_fechamento();
void imprimir_estado_mercado(TradingSystem* sistema);
void atualizar_estatisticas_mercado(TradingSystem* sistema, Ordem* ordem);
void resetar_estatisticas_diarias(TradingSystem* sistema);
void simular_abertura_mercado(TradingSystem* sistema);
void simular_fechamento_mercado(TradingSystem* sistema);

// Estruturas para pipes
typedef struct {
    int traders_to_executor[2];
    int executor_to_price_updater[2];
    int price_updater_to_arbitrage[2];
    int arbitrage_to_traders[2];
    int control_pipe[2];
    int num_pipes_criados;
    int pipes_ativos;
} SistemaPipes;

typedef struct {
    int tipo_mensagem;
    int origem_id;
    int destino_id;
    int dados_ordem;
    double valor;
    char dados_extras[100];
    time_t timestamp;
} MensagemPipe;

// Funções de pipes entre processos
int* criar_pipes_sistema();
void limpar_pipes_sistema();
int enviar_mensagem_pipe(int pipe_write, void* mensagem);
int receber_mensagem_pipe(int pipe_read, void* mensagem);
int pipes_estao_ativos();
void imprimir_status_pipes();
void testar_pipes_sistema();

// Funções para criar mensagens
MensagemPipe criar_mensagem_ordem(int trader_id, int acao_id, char tipo, double preco, int quantidade);
MensagemPipe criar_mensagem_atualizacao_preco(int acao_id, double preco_anterior, double preco_novo);
MensagemPipe criar_mensagem_arbitragem(int acao1_id, int acao2_id, double diferenca, double percentual);
MensagemPipe criar_mensagem_controle(int comando, int origem_id, int destino_id);
void imprimir_mensagem(MensagemPipe* mensagem);

// Funções de utilidade
double gerar_preco_aleatorio(double min, double max);
int gerar_id_aleatorio();
void log_evento(const char* mensagem);
void limpar_tela();
void atualizar_todos_precos(TradingSystem* sistema);
void simular_noticia_mercado(TradingSystem* sistema);

// Funções para perfis de trader
void inicializar_perfis_trader();
PerfilTrader* obter_perfil_trader(int perfil_id);
void aplicar_perfil_trader(TradingSystem* sistema, int trader_id, int perfil_id);
void log_ordem_trader(int trader_id, int acao_id, char tipo, double preco, int quantidade, const char* motivo);
void processo_trader_melhorado(int trader_id, int perfil_id);
int gerar_intervalo_aleatorio(int min, int max);
int decidir_acao_trader(TradingSystem* sistema, int trader_id, PerfilTrader* perfil);
double calcular_probabilidade_compra(TradingSystem* sistema, int acao_id, PerfilTrader* perfil);
double calcular_probabilidade_venda(TradingSystem* sistema, int acao_id, PerfilTrader* perfil);

// Funções para executor melhorado
void processo_executor_melhorado();
int ler_ordem_pipe(int pipe_read, Ordem* ordem);
int enviar_resultado_price_updater(int pipe_write, Ordem* ordem, int resultado);
int simular_tempo_processamento();
int decidir_aceitar_ordem(TradingSystem* sistema, Ordem* ordem);
double calcular_volatilidade_acao(TradingSystem* sistema, int acao_id);
int verificar_criterios_avancados(TradingSystem* sistema, Ordem* ordem);
void log_execucao_ordem(Ordem* ordem, int resultado, double tempo_processamento);
void atualizar_contadores_executor(TradingSystem* sistema, int resultado);
void executar_ordem_aceita(TradingSystem* sistema, Ordem* ordem);

// Funções para price updater melhorado
void processo_price_updater_melhorado();
int receber_notificacao_transacao(int pipe_read, Ordem* ordem, int* resultado);
double calcular_preco_media_ponderada(double preco_atual, double preco_transacao, int volume);
int validar_preco(double preco, double preco_anterior);
void atualizar_estatisticas_acao(TradingSystem* sistema, int acao_id, double novo_preco);
void enviar_atualizacao_arbitragem(int pipe_write, int acao_id, double preco_anterior, double novo_preco);
void salvar_historico_precos(TradingSystem* sistema);
void log_atualizacao_preco(int acao_id, double preco_anterior, double novo_preco, const char* motivo);
void inicializar_arquivo_historico();

// Funções para threads
void inicializar_estruturas_globais();
void limpar_estruturas_globais();
int criar_thread_trader(int trader_id, int perfil_id);
int criar_thread_executor();
int criar_thread_price_updater();
int criar_thread_arbitrage_monitor();
void* thread_trader_func(void* arg);
void* thread_executor_func(void* arg);
void* thread_price_updater_func(void* arg);
void* thread_arbitrage_monitor_func(void* arg);
int adicionar_ordem_fila(Ordem ordem);
int remover_ordem_fila(Ordem* ordem);
void parar_todas_threads();
int verificar_retorno_pthread(int resultado, const char* operacao);
void aguardar_threads_terminarem();

// Funções para demo de race conditions
void demo_race_conditions();
void detectar_inconsistencias();
void inicializar_dados_race_conditions();
void executar_demo_race_conditions();
void executar_multiplas_vezes(int num_execucoes);
void demonstrar_tipos_race_conditions();

// Funções para detector de arbitragem
void inicializar_estatisticas_arbitragem();
double calcular_spread(double preco1, double preco2);
double calcular_lucro_potencial(double preco_compra, double preco_venda, int volume);
void detectar_oportunidades_arbitragem(TradingSystem* sistema);
void executar_arbitragem_detector(TradingSystem* sistema, void* oportunidade);
void processar_oportunidades_pendentes(TradingSystem* sistema);
void exibir_estatisticas_arbitragem();
void exibir_oportunidades_ativas();
void* thread_arbitragem_detector(void* arg);
int criar_thread_arbitragem_detector(TradingSystem* sistema);
void parar_detector_arbitragem();

// Funções para race condition logger
void inicializar_race_condition_logger();
void get_precise_timestamp(time_t* timestamp, long* microsec);
char* format_timestamp(time_t timestamp, long microsec);
void log_operation(int thread_id, const char* operation_type, const char* data_type, 
                  int data_id, double old_value, double new_value, const char* details);
void detectar_race_condition_tempo_real(int thread_id, const char* operation, 
                                       const char* data_type, int data_id, 
                                       double old_value, double new_value);
void gerar_relatorio_diferencas_execucoes();
void analisar_padroes_race_conditions();
void finalizar_race_condition_logger();
void executar_multiplas_vezes_com_logging(int num_execucoes);
void comparar_arquivos_log(int num_execucoes);
int logging_esta_ativo();

// Funções para métricas de performance
void inicializar_metricas_performance();
void get_monotonic_time(struct timespec* ts);
double calculate_time_diff_ms(struct timespec start, struct timespec end);
double calculate_time_diff_us(struct timespec start, struct timespec end);
void get_resource_usage(void* metric);
void iniciar_medicao_criacao(int is_process);
void finalizar_medicao_criacao(int is_process);
void* iniciar_medicao_processamento(int is_process);
void finalizar_medicao_processamento(int is_process, int order_accepted);
void* iniciar_medicao_resposta_end_to_end(int is_process);
void finalizar_medicao_resposta_end_to_end(int is_process);
void coletar_estatisticas_recursos(int is_process);
void calcular_throughput(int is_process, double total_time_seconds);
void calcular_metricas_mercado(TradingSystem* sistema);
void coletar_estatisticas_individual(int thread_id, int is_process, int orders_processed, 
                                   double avg_latency, double throughput);
void exibir_metricas_performance(int is_process);
void exibir_metricas_mercado();
void comparar_processos_vs_threads();
void salvar_metricas_arquivo(const char* filename);
void finalizar_metricas_performance();
void* obter_metricas_processos();
void* obter_metricas_threads();
void* obter_metricas_mercado();

// Estruturas para comunicação entre processos/threads
typedef struct {
    int tipo_mensagem; // 1: nova ordem, 2: atualizar preço, 3: executar ordem
    int id_origem;
    int id_destino;
    double valor;
    int quantidade;
    char dados_extras[100];
} Mensagem;

// Variáveis globais para memória compartilhada (externas)
extern int shm_id;
extern int shm_id_pipes;

// Variável global para threads
extern TradingSystem* sistema_global;

#endif // TRADING_SYSTEM_H 