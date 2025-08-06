#include "trading_system.h"
#include <pthread.h>
#include <time.h>

// Estruturas globais compartilhadas
static FilaOrdens fila_ordens;
static EstadoMercado estado_mercado;
TradingSystem* sistema_global = NULL;

// Threads ativas
static pthread_t threads_traders[MAX_TRADERS];
static pthread_t thread_executor;
static pthread_t thread_price_updater;
static pthread_t thread_arbitrage_monitor;

// Status das threads
static int threads_traders_ativas[MAX_TRADERS] = {0};
static int thread_executor_ativa = 0;
static int thread_price_updater_ativa = 0;
static int thread_arbitrage_monitor_ativa = 0;

// Estruturas para passagem de parâmetros
typedef struct {
    int trader_id;
    int perfil_id;
    TradingSystem* sistema;
} ParametrosTrader;

typedef struct {
    TradingSystem* sistema;
} ParametrosExecutor;

typedef struct {
    TradingSystem* sistema;
} ParametrosPriceUpdater;

typedef struct {
    TradingSystem* sistema;
} ParametrosArbitrageMonitor;

// Função para inicializar estruturas globais
void inicializar_estruturas_globais() {
    printf("=== INICIALIZANDO ESTRUTURAS GLOBAIS PARA THREADS ===\n");
    
    // Inicializar fila de ordens
    fila_ordens.inicio = 0;
    fila_ordens.fim = 0;
    fila_ordens.tamanho = 0;
    pthread_mutex_init(&fila_ordens.mutex, NULL);
    pthread_cond_init(&fila_ordens.cond_nao_vazia, NULL);
    pthread_cond_init(&fila_ordens.cond_nao_cheia, NULL);
    
    // Inicializar estado do mercado
    estado_mercado.sistema_ativo = 1;
    estado_mercado.mercado_aberto = 1;
    estado_mercado.inicio_sessao = time(NULL);
    pthread_mutex_init(&estado_mercado.mutex, NULL);
    
    printf("✓ Fila de ordens inicializada (capacidade: %d)\n", MAX_FILA_ORDENS);
    printf("✓ Estado do mercado inicializado\n");
    printf("✓ Mutexes e condition variables criados\n");
}

// Função para limpar estruturas globais
void limpar_estruturas_globais() {
    printf("=== LIMPANDO ESTRUTURAS GLOBAIS ===\n");
    
    // Destruir mutexes e condition variables
    pthread_mutex_destroy(&fila_ordens.mutex);
    pthread_cond_destroy(&fila_ordens.cond_nao_vazia);
    pthread_cond_destroy(&fila_ordens.cond_nao_cheia);
    pthread_mutex_destroy(&estado_mercado.mutex);
    
    printf("✓ Estruturas globais limpas\n");
}

// Função para verificar retorno das funções pthread
int verificar_retorno_pthread(int resultado, const char* operacao) {
    if (resultado != 0) {
        printf("ERRO: Falha na operação pthread '%s' - Código: %d (%s)\n", 
               operacao, resultado, strerror(resultado));
        return 0;
    }
    return 1;
}

// Função para adicionar ordem na fila
int adicionar_ordem_fila(Ordem ordem) {
    pthread_mutex_lock(&fila_ordens.mutex);
    
    // Verificar se fila está cheia
    while (fila_ordens.tamanho >= MAX_FILA_ORDENS) {
        printf("AVISO: Fila de ordens cheia, aguardando espaço...\n");
        pthread_cond_wait(&fila_ordens.cond_nao_cheia, &fila_ordens.mutex);
    }
    
    // Adicionar ordem
    fila_ordens.ordens[fila_ordens.fim] = ordem;
    fila_ordens.fim = (fila_ordens.fim + 1) % MAX_FILA_ORDENS;
    fila_ordens.tamanho++;
    
    printf("✓ Ordem adicionada na fila (Trader %d, Ação %d, Tipo: %c, Preço: %.2f, Qtd: %d)\n",
           ordem.trader_id, ordem.acao_id, ordem.tipo, ordem.preco, ordem.quantidade);
    
    // Sinalizar que há ordens disponíveis
    pthread_cond_signal(&fila_ordens.cond_nao_vazia);
    
    pthread_mutex_unlock(&fila_ordens.mutex);
    return 1;
}

// Função para remover ordem da fila
int remover_ordem_fila(Ordem* ordem) {
    pthread_mutex_lock(&fila_ordens.mutex);
    
    // Verificar se fila está vazia
    while (fila_ordens.tamanho == 0) {
        pthread_cond_wait(&fila_ordens.cond_nao_vazia, &fila_ordens.mutex);
    }
    
    // Remover ordem
    *ordem = fila_ordens.ordens[fila_ordens.inicio];
    fila_ordens.inicio = (fila_ordens.inicio + 1) % MAX_FILA_ORDENS;
    fila_ordens.tamanho--;
    
    // Sinalizar que há espaço disponível
    pthread_cond_signal(&fila_ordens.cond_nao_cheia);
    
    pthread_mutex_unlock(&fila_ordens.mutex);
    return 1;
}

// Função da thread trader
void* thread_trader_func(void* arg) {
    ParametrosTrader* params = (ParametrosTrader*)arg;
    int trader_id = params->trader_id;
    int perfil_id = params->perfil_id;
    TradingSystem* sistema = params->sistema;
    
    printf("=== THREAD TRADER %d INICIADA (Perfil: %d) ===\n", trader_id, perfil_id);
    
    // Obter perfil do trader
    PerfilTrader* perfil = obter_perfil_trader(perfil_id);
    if (!perfil) {
        printf("ERRO: Perfil inválido %d para trader %d\n", perfil_id, trader_id);
        free(params);
        return NULL;
    }
    
    printf("Trader %d iniciado com perfil '%s'\n", trader_id, perfil->nome);
    printf("Configurações: intervalo %d-%ds, max %d ordens, tempo limite %ds\n",
           perfil->intervalo_min_ordens, perfil->intervalo_max_ordens,
           perfil->max_ordens_por_sessao, perfil->tempo_limite_sessao);
    
    // Variáveis de controle e métricas
    int ordens_enviadas = 0;
    time_t inicio_sessao = time(NULL);
    int orders_processed = 0;
    double total_latency = 0.0;
    
    while (estado_mercado.sistema_ativo) {
        // Verificar limites de sessão
        time_t tempo_atual = time(NULL);
        if (tempo_atual - inicio_sessao > perfil->tempo_limite_sessao) {
            printf("Trader %d: Tempo limite de sessão atingido\n", trader_id);
            break;
        }
        
        if (ordens_enviadas >= perfil->max_ordens_por_sessao) {
            printf("Trader %d: Limite de ordens atingido (%d/%d)\n", 
                   trader_id, ordens_enviadas, perfil->max_ordens_por_sessao);
            break;
        }
        
        // Iniciar medição de tempo de processamento
        void* processing_time = iniciar_medicao_processamento(0); // 0 = threads
        
        // Decidir ação do trader
        int acao_id = decidir_acao_trader(sistema, trader_id, perfil);
        if (acao_id >= 0) {
            // Gerar ordem
            Ordem ordem;
            ordem.id = gerar_id_aleatorio();
            ordem.trader_id = trader_id;
            ordem.acao_id = acao_id;
            ordem.timestamp = time(NULL);
            ordem.status = 0; // Pendente
            
            // Decidir tipo de ordem (compra/venda)
            double prob_compra = calcular_probabilidade_compra(sistema, acao_id, perfil);
            double random = (double)rand() / RAND_MAX;
            
            if (random < prob_compra) {
                ordem.tipo = 'C'; // Compra
                ordem.preco = sistema->acoes[acao_id].preco_atual * (1.0 + (rand() % 100 - 50) / 10000.0);
                ordem.quantidade = (int)(perfil->volume_medio * (0.5 + (double)rand() / RAND_MAX));
                
                printf("NOVA ORDEM: Trader %d compra %d ações de %s a R$ %.2f\n",
                       trader_id, ordem.quantidade, sistema->acoes[acao_id].nome, ordem.preco);
                log_ordem_trader(trader_id, acao_id, 'C', ordem.preco, ordem.quantidade, "Probabilidade de compra");
            } else {
                ordem.tipo = 'V'; // Venda
                ordem.preco = sistema->acoes[acao_id].preco_atual * (1.0 + (rand() % 100 - 50) / 10000.0);
                ordem.quantidade = (int)(perfil->volume_medio * (0.5 + (double)rand() / RAND_MAX));
                
                printf("NOVA ORDEM: Trader %d vende %d ações de %s a R$ %.2f\n",
                       trader_id, ordem.quantidade, sistema->acoes[acao_id].nome, ordem.preco);
                log_ordem_trader(trader_id, acao_id, 'V', ordem.preco, ordem.quantidade, "Probabilidade de venda");
            }
            
            // Adicionar ordem na fila global
            int order_accepted = adicionar_ordem_fila(ordem);
            
            // Finalizar medição de tempo de processamento
            finalizar_medicao_processamento(0, order_accepted); // 0 = threads
            
            if (order_accepted) {
                ordens_enviadas++;
                orders_processed++;
                // total_latency será calculado na função finalizar_medicao_processamento
                
                printf("Trader %d: Ordem criada (total: %d/%d)\n", 
                       trader_id, ordens_enviadas, perfil->max_ordens_por_sessao);
            }
        }
        
        // Aguardar intervalo
        int intervalo = gerar_intervalo_aleatorio(perfil->intervalo_min_ordens, perfil->intervalo_max_ordens);
        sleep(intervalo);
    }
    
    // Coletar estatísticas individuais
    double avg_latency = orders_processed > 0 ? total_latency / orders_processed : 0.0;
    double throughput = orders_processed / 30.0; // Estimativa de 30 segundos
    coletar_estatisticas_individual(trader_id, 0, orders_processed, avg_latency, throughput);
    
    printf("=== THREAD TRADER %d FINALIZADA ===\n", trader_id);
    printf("Total de ordens enviadas: %d\n", ordens_enviadas);
    
    free(params);
    return NULL;
}

// Função da thread executor
void* thread_executor_func(void* arg) {
    ParametrosExecutor* params = (ParametrosExecutor*)arg;
    TradingSystem* sistema = params->sistema;
    
    printf("=== THREAD EXECUTOR INICIADA ===\n");
    
    while (estado_mercado.sistema_ativo) {
        // Remover ordem da fila
        Ordem ordem;
        if (remover_ordem_fila(&ordem)) {
            printf("EXECUTOR: Processando ordem do Trader %d\n", ordem.trader_id);
            
            // Simular tempo de processamento
            int tempo_processamento = simular_tempo_processamento();
            
            // Decidir se aceita ou rejeita a ordem
            int resultado = decidir_aceitar_ordem(sistema, &ordem);
            
            // Log da execução
            log_execucao_ordem(&ordem, resultado, tempo_processamento);
            
            // Atualizar contadores
            atualizar_contadores_executor(sistema, resultado);
            
            // Se aceitou, executar a ordem
            if (resultado) {
                executar_ordem_aceita(sistema, &ordem);
            }
        }
        
        // Pequena pausa
        usleep(100000); // 100ms
    }
    
    printf("=== THREAD EXECUTOR FINALIZADA ===\n");
    
    free(params);
    return NULL;
}

// Função da thread price updater
void* thread_price_updater_func(void* arg) {
    ParametrosPriceUpdater* params = (ParametrosPriceUpdater*)arg;
    TradingSystem* sistema = params->sistema;
    
    printf("=== THREAD PRICE UPDATER INICIADA ===\n");
    
    // Inicializar arquivo de histórico
    inicializar_arquivo_historico();
    
    int contador_snapshot = 0;
    
    while (estado_mercado.sistema_ativo) {
        // Atualização periódica de preços (simulação de mercado)
        static int contador_periodico = 0;
        contador_periodico++;
        
        if (contador_periodico >= 30) { // A cada 30 iterações (3 segundos)
            contador_periodico = 0;
            
            // Atualizar preços de todas as ações
            for (int i = 0; i < sistema->num_acoes; i++) {
                Acao* acao = &sistema->acoes[i];
                double preco_anterior = acao->preco_atual;
                
                // Simular variação de mercado
                double variacao = (rand() % 200 - 100) / 10000.0; // ±1%
                double novo_preco = preco_anterior * (1.0 + variacao);
                
                if (validar_preco(novo_preco, preco_anterior)) {
                    atualizar_estatisticas_acao(sistema, i, novo_preco);
                    log_atualizacao_preco(i, preco_anterior, novo_preco, "Variação de mercado");
                }
            }
            
            // Salvar snapshot a cada 10 atualizações periódicas
            contador_snapshot++;
            if (contador_snapshot >= 10) {
                salvar_historico_precos(sistema);
                contador_snapshot = 0;
                printf("PRICE UPDATER: Snapshot salvo no arquivo de histórico\n");
            }
        }
        
        // Pequena pausa
        usleep(100000); // 100ms
    }
    
    printf("=== THREAD PRICE UPDATER FINALIZADA ===\n");
    
    free(params);
    return NULL;
}

// Função da thread arbitrage monitor
void* thread_arbitrage_monitor_func(void* arg) {
    ParametrosArbitrageMonitor* params = (ParametrosArbitrageMonitor*)arg;
    TradingSystem* sistema = params->sistema;
    
    printf("=== THREAD ARBITRAGE MONITOR INICIADA ===\n");
    
    while (estado_mercado.sistema_ativo) {
        // Monitorar arbitragem
        monitorar_arbitragem(sistema);
        detectar_padroes_preco(sistema);
        
        // Simular eventos de mercado ocasionalmente
        if (rand() % 100 < 5) { // 5% de chance
            simular_evento_mercado(sistema);
        }
        
        sleep(5); // Monitorar a cada 5 segundos
    }
    
    printf("=== THREAD ARBITRAGE MONITOR FINALIZADA ===\n");
    
    free(params);
    return NULL;
}

// Função para criar thread trader
int criar_thread_trader(int trader_id, int perfil_id) {
    if (trader_id < 0 || trader_id >= MAX_TRADERS) {
        printf("ERRO: ID de trader inválido: %d\n", trader_id);
        return 0;
    }
    
    if (threads_traders_ativas[trader_id]) {
        printf("AVISO: Thread trader %d já está ativa\n", trader_id);
        return 0;
    }
    
    // Alocar parâmetros
    ParametrosTrader* params = malloc(sizeof(ParametrosTrader));
    if (!params) {
        printf("ERRO: Falha ao alocar memória para parâmetros do trader %d\n", trader_id);
        return 0;
    }
    
    params->trader_id = trader_id;
    params->perfil_id = perfil_id;
    params->sistema = sistema_global;
    
    // Criar thread
    int resultado = pthread_create(&threads_traders[trader_id], NULL, thread_trader_func, params);
    
    if (!verificar_retorno_pthread(resultado, "pthread_create trader")) {
        free(params);
        return 0;
    }
    
    threads_traders_ativas[trader_id] = 1;
    printf("✓ Thread trader %d criada com sucesso\n", trader_id);
    
    return 1;
}

// Função para criar thread executor
int criar_thread_executor() {
    if (thread_executor_ativa) {
        printf("AVISO: Thread executor já está ativa\n");
        return 0;
    }
    
    // Alocar parâmetros
    ParametrosExecutor* params = malloc(sizeof(ParametrosExecutor));
    if (!params) {
        printf("ERRO: Falha ao alocar memória para parâmetros do executor\n");
        return 0;
    }
    
    params->sistema = sistema_global;
    
    // Criar thread
    int resultado = pthread_create(&thread_executor, NULL, thread_executor_func, params);
    
    if (!verificar_retorno_pthread(resultado, "pthread_create executor")) {
        free(params);
        return 0;
    }
    
    thread_executor_ativa = 1;
    printf("✓ Thread executor criada com sucesso\n");
    
    return 1;
}

// Função para criar thread price updater
int criar_thread_price_updater() {
    if (thread_price_updater_ativa) {
        printf("AVISO: Thread price updater já está ativa\n");
        return 0;
    }
    
    // Alocar parâmetros
    ParametrosPriceUpdater* params = malloc(sizeof(ParametrosPriceUpdater));
    if (!params) {
        printf("ERRO: Falha ao alocar memória para parâmetros do price updater\n");
        return 0;
    }
    
    params->sistema = sistema_global;
    
    // Criar thread
    int resultado = pthread_create(&thread_price_updater, NULL, thread_price_updater_func, params);
    
    if (!verificar_retorno_pthread(resultado, "pthread_create price updater")) {
        free(params);
        return 0;
    }
    
    thread_price_updater_ativa = 1;
    printf("✓ Thread price updater criada com sucesso\n");
    
    return 1;
}

// Função para criar thread arbitrage monitor
int criar_thread_arbitrage_monitor() {
    if (thread_arbitrage_monitor_ativa) {
        printf("AVISO: Thread arbitrage monitor já está ativa\n");
        return 0;
    }
    
    // Alocar parâmetros
    ParametrosArbitrageMonitor* params = malloc(sizeof(ParametrosArbitrageMonitor));
    if (!params) {
        printf("ERRO: Falha ao alocar memória para parâmetros do arbitrage monitor\n");
        return 0;
    }
    
    params->sistema = sistema_global;
    
    // Criar thread
    int resultado = pthread_create(&thread_arbitrage_monitor, NULL, thread_arbitrage_monitor_func, params);
    
    if (!verificar_retorno_pthread(resultado, "pthread_create arbitrage monitor")) {
        free(params);
        return 0;
    }
    
    thread_arbitrage_monitor_ativa = 1;
    printf("✓ Thread arbitrage monitor criada com sucesso\n");
    
    return 1;
}

// Função para parar todas as threads
void parar_todas_threads() {
    printf("=== PARANDO TODAS AS THREADS ===\n");
    
    pthread_mutex_lock(&estado_mercado.mutex);
    estado_mercado.sistema_ativo = 0;
    pthread_mutex_unlock(&estado_mercado.mutex);
    
    printf("✓ Sinal de parada enviado para todas as threads\n");
}

// Função para aguardar threads terminarem
void aguardar_threads_terminarem() {
    printf("=== AGUARDANDO THREADS TERMINAREM ===\n");
    
    // Aguardar threads traders
    for (int i = 0; i < MAX_TRADERS; i++) {
        if (threads_traders_ativas[i]) {
            printf("Aguardando thread trader %d...\n", i);
            int resultado = pthread_join(threads_traders[i], NULL);
            if (verificar_retorno_pthread(resultado, "pthread_join trader")) {
                threads_traders_ativas[i] = 0;
                printf("✓ Thread trader %d finalizada\n", i);
            }
        }
    }
    
    // Aguardar thread executor
    if (thread_executor_ativa) {
        printf("Aguardando thread executor...\n");
        int resultado = pthread_join(thread_executor, NULL);
        if (verificar_retorno_pthread(resultado, "pthread_join executor")) {
            thread_executor_ativa = 0;
            printf("✓ Thread executor finalizada\n");
        }
    }
    
    // Aguardar thread price updater
    if (thread_price_updater_ativa) {
        printf("Aguardando thread price updater...\n");
        int resultado = pthread_join(thread_price_updater, NULL);
        if (verificar_retorno_pthread(resultado, "pthread_join price updater")) {
            thread_price_updater_ativa = 0;
            printf("✓ Thread price updater finalizada\n");
        }
    }
    
    // Aguardar thread arbitrage monitor
    if (thread_arbitrage_monitor_ativa) {
        printf("Aguardando thread arbitrage monitor...\n");
        int resultado = pthread_join(thread_arbitrage_monitor, NULL);
        if (verificar_retorno_pthread(resultado, "pthread_join arbitrage monitor")) {
            thread_arbitrage_monitor_ativa = 0;
            printf("✓ Thread arbitrage monitor finalizada\n");
        }
    }
    
    printf("✓ Todas as threads finalizadas\n");
} 