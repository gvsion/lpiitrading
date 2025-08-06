#include "trading_system.h"
#include <poll.h>
#include <sys/time.h>

// Contadores específicos do executor
static int total_ordens_processadas = 0;
static int ordens_aceitas = 0;
static int ordens_rejeitadas = 0;
static int ordens_timeout = 0;

// Função para simular tempo de processamento (50-200ms)
int simular_tempo_processamento() {
    int tempo = TEMPO_PROCESSAMENTO_MIN + (rand() % (TEMPO_PROCESSAMENTO_MAX - TEMPO_PROCESSAMENTO_MIN + 1));
    usleep(tempo * 1000); // Converter para microssegundos
    return tempo;
}

// Função para calcular volatilidade de uma ação
double calcular_volatilidade_acao(TradingSystem* sistema, int acao_id) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return 0.0;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    
    // Calcular volatilidade baseada na variação percentual
    double volatilidade = fabs(acao->variacao);
    
    // Adicionar componente baseado no volume
    double componente_volume = (double)acao->volume_negociado / 1000.0;
    if (componente_volume > 0.1) componente_volume = 0.1; // Limitar a 10%
    
    volatilidade += componente_volume;
    
    return volatilidade;
}

// Função para verificar critérios avançados de rejeição
int verificar_criterios_avancados(TradingSystem* sistema, Ordem* ordem) {
    if (!ordem || ordem->acao_id < 0 || ordem->acao_id >= sistema->num_acoes) {
        return 0; // Rejeitar se dados inválidos
    }
    
    Acao* acao = &sistema->acoes[ordem->acao_id];
    
    // 1. Verificar volatilidade da ação
    double volatilidade = calcular_volatilidade_acao(sistema, ordem->acao_id);
    if (volatilidade > MAX_VOLATILIDADE_ACEITA) {
        printf("EXECUTOR: Ordem rejeitada - Volatilidade muito alta (%.2f%% > %.2f%%)\n", 
               volatilidade * 100, MAX_VOLATILIDADE_ACEITA * 100);
        return 0;
    }
    
    // 2. Verificar volume da ordem
    if (ordem->quantidade < MIN_VOLUME_ACEITO) {
        printf("EXECUTOR: Ordem rejeitada - Volume muito baixo (%d < %d)\n", 
               ordem->quantidade, MIN_VOLUME_ACEITO);
        return 0;
    }
    
    if (ordem->quantidade > MAX_VOLUME_ACEITO) {
        printf("EXECUTOR: Ordem rejeitada - Volume muito alto (%d > %d)\n", 
               ordem->quantidade, MAX_VOLUME_ACEITO);
        return 0;
    }
    
    // 3. Verificar diferença de preço
    double diferenca_preco = fabs(ordem->preco - acao->preco_atual);
    double percentual_diferenca = diferenca_preco / acao->preco_atual;
    
    if (percentual_diferenca > 0.05) { // 5% de diferença
        printf("EXECUTOR: Ordem rejeitada - Diferença de preço muito alta (%.2f%%)\n", 
               percentual_diferenca * 100);
        return 0;
    }
    
    // 4. Verificar se o trader tem saldo/ações suficientes
    Trader* trader = &sistema->traders[ordem->trader_id];
    
    if (ordem->tipo == 'C') { // Compra
        double custo_total = ordem->preco * ordem->quantidade;
        if (trader->saldo < custo_total) {
            printf("EXECUTOR: Ordem rejeitada - Saldo insuficiente (R$ %.2f < R$ %.2f)\n", 
                   trader->saldo, custo_total);
            return 0;
        }
    } else if (ordem->tipo == 'V') { // Venda
        if (trader->acoes_possuidas[ordem->acao_id] < ordem->quantidade) {
            printf("EXECUTOR: Ordem rejeitada - Ações insuficientes (%d < %d)\n", 
                   trader->acoes_possuidas[ordem->acao_id], ordem->quantidade);
            return 0;
        }
    }
    
    // 5. Verificar se a ação está muito volátil no momento
    if (fabs(acao->variacao) > 0.1) { // 10% de variação
        printf("EXECUTOR: Ordem rejeitada - Ação muito volátil (variação: %.2f%%)\n", 
               acao->variacao * 100);
        return 0;
    }
    
    return 1; // Ordem aceita
}

// Função para decidir se aceita ou rejeita a ordem
int decidir_aceitar_ordem(TradingSystem* sistema, Ordem* ordem) {
    // Verificar critérios avançados
    if (!verificar_criterios_avancados(sistema, ordem)) {
        return 0; // Rejeitar
    }
    
    // Simular alguma aleatoriedade na decisão (95% de aceitação se passar pelos critérios)
    double random = (double)rand() / RAND_MAX;
    if (random > 0.95) {
        printf("EXECUTOR: Ordem rejeitada - Decisão aleatória do sistema\n");
        return 0;
    }
    
    return 1; // Aceitar
}

// Função para ler ordem do pipe com timeout usando poll()
int ler_ordem_pipe(int pipe_read, Ordem* ordem) {
    struct pollfd pfd;
    pfd.fd = pipe_read;
    pfd.events = POLLIN;
    
    // Usar poll() com timeout
    int poll_result = poll(&pfd, 1, TIMEOUT_PIPE_READ);
    
    if (poll_result > 0 && (pfd.revents & POLLIN)) {
        // Dados disponíveis para leitura
        ssize_t bytes_lidos = read(pipe_read, ordem, sizeof(Ordem));
        if (bytes_lidos == sizeof(Ordem)) {
            return 1; // Ordem lida com sucesso
        }
    } else if (poll_result == 0) {
        // Timeout - nenhum dado disponível
        ordens_timeout++;
        return 0;
    }
    
    return -1; // Erro na leitura
}

// Função para enviar resultado para o price updater
int enviar_resultado_price_updater(int pipe_write, Ordem* ordem, int resultado) {
    MensagemPipe msg;
    msg.tipo_mensagem = 2; // Resultado de execução
    msg.origem_id = 1; // Executor
    msg.destino_id = 2; // Price Updater
    msg.dados_ordem = resultado; // 1: aceita, 0: rejeita
    msg.valor = ordem->preco;
    msg.timestamp = time(NULL);
    
    return enviar_mensagem_pipe(pipe_write, &msg);
}

// Função para log detalhado da execução
void log_execucao_ordem(Ordem* ordem, int resultado, double tempo_processamento) {
    time_t agora = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&agora));
    
    printf("[%s] EXECUTOR: %s ordem do Trader %d (%s %d ações a R$ %.2f) em %.0fms\n", 
           timestamp, 
           resultado ? "ACEITOU" : "REJEITOU",
           ordem->trader_id,
           ordem->tipo == 'C' ? "COMPRA" : "VENDA",
           ordem->quantidade,
           ordem->preco,
           tempo_processamento);
}

// Função para atualizar contadores do executor
void atualizar_contadores_executor(TradingSystem* sistema, int resultado) {
    pthread_mutex_lock(&sistema->executor.mutex);
    
    total_ordens_processadas++;
    sistema->executor.total_ordens++;
    
    if (resultado) {
        ordens_aceitas++;
        sistema->executor.ordens_executadas++;
    } else {
        ordens_rejeitadas++;
        sistema->executor.ordens_canceladas++;
    }
    
    pthread_mutex_unlock(&sistema->executor.mutex);
}

// Função principal do processo executor melhorado
void processo_executor_melhorado() {
    printf("=== PROCESSO EXECUTOR MELHORADO INICIADO (PID: %d) ===\n", getpid());
    
    // Anexar memória compartilhada
    TradingSystem* sistema = (TradingSystem*)shmat(shm_id, NULL, 0);
    if (sistema == (void*)-1) {
        perror("Erro ao anexar memória compartilhada no processo executor");
        exit(1);
    }
    
    // Anexar memória compartilhada de pipes
    SistemaPipes* pipes = (SistemaPipes*)shmat(shm_id_pipes, NULL, 0);
    if (pipes == (void*)-1) {
        perror("Erro ao anexar memória compartilhada de pipes");
        shmdt(sistema);
        exit(1);
    }
    
    printf("Executor melhorado iniciado com configurações:\n");
    printf("- Tempo de processamento: %d-%dms\n", TEMPO_PROCESSAMENTO_MIN, TEMPO_PROCESSAMENTO_MAX);
    printf("- Timeout de leitura: %dms\n", TIMEOUT_PIPE_READ);
    printf("- Volatilidade máxima aceita: %.1f%%\n", MAX_VOLATILIDADE_ACEITA * 100);
    printf("- Volume aceito: %d-%d ações\n", MIN_VOLUME_ACEITO, MAX_VOLUME_ACEITO);
    
    // Configurar poll para leitura de pipes
    struct pollfd pfd;
    pfd.fd = pipes->traders_to_executor[0]; // Pipe de leitura dos traders
    pfd.events = POLLIN;
    
    while (sistema->sistema_ativo) {
        // Verificar se há ordens para processar
        int poll_result = poll(&pfd, 1, TIMEOUT_PIPE_READ);
        
        if (poll_result > 0 && (pfd.revents & POLLIN)) {
            // Ordem disponível para processamento
            Ordem ordem;
            int resultado_leitura = ler_ordem_pipe(pipes->traders_to_executor[0], &ordem);
            
            if (resultado_leitura == 1) {
                // Ordem lida com sucesso
                printf("EXECUTOR: Nova ordem recebida do Trader %d\n", ordem.trader_id);
                
                // Simular tempo de processamento
                double tempo_processamento = simular_tempo_processamento();
                
                // Decidir se aceita ou rejeita a ordem
                int resultado = decidir_aceitar_ordem(sistema, &ordem);
                
                // Log da execução
                log_execucao_ordem(&ordem, resultado, tempo_processamento);
                
                // Atualizar contadores
                atualizar_contadores_executor(sistema, resultado);
                
                // Enviar resultado para o price updater
                if (enviar_resultado_price_updater(pipes->executor_to_price_updater[1], &ordem, resultado) > 0) {
                    printf("EXECUTOR: Resultado enviado para Price Updater\n");
                }
                
                // Se aceitou, executar a ordem
                if (resultado) {
                    executar_ordem_aceita(sistema, &ordem);
                }
                
            } else if (resultado_leitura == 0) {
                // Timeout - nenhuma ordem disponível
                // Continuar loop
            } else {
                // Erro na leitura
                printf("EXECUTOR: Erro ao ler ordem do pipe\n");
            }
        } else if (poll_result == 0) {
            // Timeout - nenhuma ordem disponível
            // Continuar loop
        } else {
            // Erro no poll
            printf("EXECUTOR: Erro no poll()\n");
        }
        
        // Pequena pausa para não sobrecarregar
        usleep(10000); // 10ms
    }
    
    // Estatísticas finais
    printf("=== EXECUTOR MELHORADO FINALIZADO ===\n");
    printf("Total de ordens processadas: %d\n", total_ordens_processadas);
    printf("Ordens aceitas: %d (%.1f%%)\n", ordens_aceitas, 
           total_ordens_processadas > 0 ? (double)ordens_aceitas / total_ordens_processadas * 100 : 0);
    printf("Ordens rejeitadas: %d (%.1f%%)\n", ordens_rejeitadas,
           total_ordens_processadas > 0 ? (double)ordens_rejeitadas / total_ordens_processadas * 100 : 0);
    printf("Timeouts de leitura: %d\n", ordens_timeout);
    
    // Desanexar memória compartilhada
    shmdt(sistema);
    shmdt(pipes);
    
    exit(0);
}

// Função para executar ordem aceita
void executar_ordem_aceita(TradingSystem* sistema, Ordem* ordem) {
    if (!ordem || ordem->trader_id < 0 || ordem->trader_id >= MAX_TRADERS ||
        ordem->acao_id < 0 || ordem->acao_id >= sistema->num_acoes) {
        return;
    }
    
    Trader* trader = &sistema->traders[ordem->trader_id];
    Acao* acao = &sistema->acoes[ordem->acao_id];
    
    pthread_mutex_lock(&trader->mutex);
    pthread_mutex_lock(&acao->mutex);
    
    if (ordem->tipo == 'C') { // Ordem de compra
        double custo_total = ordem->preco * ordem->quantidade;
        trader->saldo -= custo_total;
        trader->acoes_possuidas[ordem->acao_id] += ordem->quantidade;
        acao->volume_negociado += ordem->quantidade;
        
        printf("EXECUTADA: Trader %d comprou %d ações de %s a R$ %.2f\n", 
               ordem->trader_id, ordem->quantidade, acao->nome, ordem->preco);
        
    } else if (ordem->tipo == 'V') { // Ordem de venda
        double valor_recebido = ordem->preco * ordem->quantidade;
        trader->saldo += valor_recebido;
        trader->acoes_possuidas[ordem->acao_id] -= ordem->quantidade;
        acao->volume_negociado += ordem->quantidade;
        
        printf("EXECUTADA: Trader %d vendeu %d ações de %s a R$ %.2f\n", 
               ordem->trader_id, ordem->quantidade, acao->nome, ordem->preco);
    }
    
    pthread_mutex_unlock(&acao->mutex);
    pthread_mutex_unlock(&trader->mutex);
} 