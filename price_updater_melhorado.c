#include "trading_system.h"
#include <poll.h>
#include <sys/time.h>

// Contadores específicos do price updater
static int total_atualizacoes = 0;
static int atualizacoes_validas = 0;
static int atualizacoes_rejeitadas = 0;
static int notificacoes_recebidas = 0;

// Função para inicializar arquivo de histórico
void inicializar_arquivo_historico() {
    FILE* arquivo = fopen(ARQUIVO_HISTORICO, "w");
    if (arquivo) {
        fprintf(arquivo, "=== HISTÓRICO DE PREÇOS ===\n");
        fprintf(arquivo, "Timestamp,Ação,Preço_Anterior,Preço_Novo,Variação,Motivo\n");
        fclose(arquivo);
        printf("PRICE UPDATER: Arquivo de histórico inicializado: %s\n", ARQUIVO_HISTORICO);
    } else {
        printf("PRICE UPDATER: Erro ao criar arquivo de histórico\n");
    }
}

// Função para receber notificação de transação via pipe
int receber_notificacao_transacao(int pipe_read, Ordem* ordem, int* resultado) {
    struct pollfd pfd;
    pfd.fd = pipe_read;
    pfd.events = POLLIN;
    
    // Usar poll() com timeout
    int poll_result = poll(&pfd, 1, 100); // 100ms timeout
    
    if (poll_result > 0 && (pfd.revents & POLLIN)) {
        // Dados disponíveis para leitura
        MensagemPipe msg;
        ssize_t bytes_lidos = read(pipe_read, &msg, sizeof(MensagemPipe));
        
        if (bytes_lidos == sizeof(MensagemPipe) && msg.tipo_mensagem == 2) {
            // Converter mensagem para ordem
            ordem->trader_id = msg.origem_id;
            ordem->acao_id = msg.dados_ordem;
            ordem->preco = msg.valor;
            ordem->timestamp = msg.timestamp;
            *resultado = msg.dados_ordem; // 1: aceita, 0: rejeita
            
            notificacoes_recebidas++;
            return 1; // Notificação recebida com sucesso
        }
    }
    
    return 0; // Nenhuma notificação disponível
}

// Função para calcular preço usando média ponderada
double calcular_preco_media_ponderada(double preco_atual, double preco_transacao, int volume) {
    // Calcular peso baseado no volume (volume maior = mais peso)
    double peso_volume = (double)volume / 1000.0; // Normalizar volume
    if (peso_volume > 1.0) peso_volume = 1.0; // Limitar a 100%
    
    // Ajustar pesos baseado no volume
    double peso_transacao = PESO_ULTIMA_TRANSACAO * (0.5 + peso_volume * 0.5);
    double peso_atual = PESO_PRECO_ATUAL * (1.0 - peso_volume * 0.3);
    
    // Calcular média ponderada
    double novo_preco = (preco_transacao * peso_transacao + preco_atual * peso_atual) / (peso_transacao + peso_atual);
    
    return novo_preco;
}

// Função para validar preço (evitar preços negativos ou muito voláteis)
int validar_preco(double preco, double preco_anterior) {
    // Verificar preço mínimo
    if (preco < MIN_PRECO_ACAO) {
        printf("PRICE UPDATER: Preço rejeitado - Muito baixo (R$ %.2f < R$ %.2f)\n", 
               preco, MIN_PRECO_ACAO);
        return 0;
    }
    
    // Verificar preço máximo
    if (preco > MAX_PRECO_ACAO) {
        printf("PRICE UPDATER: Preço rejeitado - Muito alto (R$ %.2f > R$ %.2f)\n", 
               preco, MAX_PRECO_ACAO);
        return 0;
    }
    
    // Verificar variação máxima
    if (preco_anterior > 0) {
        double variacao = fabs(preco - preco_anterior) / preco_anterior;
        if (variacao > MAX_VARIACAO_PRECO) {
            printf("PRICE UPDATER: Preço rejeitado - Variação muito alta (%.2f%% > %.2f%%)\n", 
                   variacao * 100, MAX_VARIACAO_PRECO * 100);
            return 0;
        }
    }
    
    return 1; // Preço válido
}

// Função para atualizar estatísticas da ação
void atualizar_estatisticas_acao(TradingSystem* sistema, int acao_id, double novo_preco) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    
    pthread_mutex_lock(&acao->mutex);
    
    // Atualizar preços
    acao->preco_anterior = acao->preco_atual;
    acao->preco_atual = novo_preco;
    acao->variacao = (novo_preco - acao->preco_anterior) / acao->preco_anterior;
    
    // Atualizar estatísticas
    if (novo_preco > acao->preco_maximo) {
        acao->preco_maximo = novo_preco;
    }
    if (novo_preco < acao->preco_minimo || acao->preco_minimo == 0) {
        acao->preco_minimo = novo_preco;
    }
    
    // Atualizar volume negociado (simulado)
    acao->volume_negociado += rand() % 100 + 50; // 50-150 ações
    
    // Atualizar número de operações
    acao->num_operacoes++;
    
    // Atualizar variação diária
    acao->variacao_diaria = (novo_preco - acao->preco_anterior) / acao->preco_anterior;
    
    pthread_mutex_unlock(&acao->mutex);
}

// Função para enviar atualização para monitor de arbitragem
void enviar_atualizacao_arbitragem(int pipe_write, int acao_id, double preco_anterior, double novo_preco) {
    (void)preco_anterior; // Evitar warning de parâmetro não utilizado
    MensagemPipe msg;
    msg.tipo_mensagem = 3; // Atualização de preço
    msg.origem_id = 2; // Price Updater
    msg.destino_id = 3; // Arbitrage Monitor
    msg.dados_ordem = acao_id;
    msg.valor = novo_preco;
    msg.timestamp = time(NULL);
    
    if (enviar_mensagem_pipe(pipe_write, &msg) > 0) {
        printf("PRICE UPDATER: Atualização enviada para Arbitrage Monitor (Ação %d)\n", acao_id);
    }
}

// Função para salvar histórico de preços em arquivo
void salvar_historico_precos(TradingSystem* sistema) {
    FILE* arquivo = fopen(ARQUIVO_HISTORICO, "a");
    if (!arquivo) {
        printf("PRICE UPDATER: Erro ao abrir arquivo de histórico\n");
        return;
    }
    
    time_t agora = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&agora));
    
    fprintf(arquivo, "=== SNAPSHOT %s ===\n", timestamp);
    fprintf(arquivo, "Ação,Preço_Atual,Variação,Volume,Max_Dia,Min_Dia,Operações\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        fprintf(arquivo, "%s,%.2f,%.2f%%,%d,%.2f,%.2f,%d\n",
                acao->nome,
                acao->preco_atual,
                acao->variacao * 100,
                acao->volume_negociado,
                acao->preco_maximo,
                acao->preco_minimo,
                acao->num_operacoes);
    }
    
    fprintf(arquivo, "\n");
    fclose(arquivo);
}

// Função para log detalhado de atualização de preço
void log_atualizacao_preco(int acao_id, double preco_anterior, double novo_preco, const char* motivo) {
    time_t agora = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&agora));
    
    double variacao = (novo_preco - preco_anterior) / preco_anterior * 100;
    
    printf("[%s] PRICE UPDATER: Ação %d - R$ %.2f → R$ %.2f (%.2f%%) - %s\n", 
           timestamp, acao_id, preco_anterior, novo_preco, variacao, motivo);
}

// Função principal do processo price updater melhorado
void processo_price_updater_melhorado() {
    printf("=== PROCESSO PRICE UPDATER MELHORADO INICIADO (PID: %d) ===\n", getpid());
    
    // Anexar memória compartilhada
    TradingSystem* sistema = (TradingSystem*)shmat(shm_id, NULL, 0);
    if (sistema == (void*)-1) {
        perror("Erro ao anexar memória compartilhada no processo price updater");
        exit(1);
    }
    
    // Anexar memória compartilhada de pipes
    SistemaPipes* pipes = (SistemaPipes*)shmat(shm_id_pipes, NULL, 0);
    if (pipes == (void*)-1) {
        perror("Erro ao anexar memória compartilhada de pipes");
        shmdt(sistema);
        exit(1);
    }
    
    // Inicializar arquivo de histórico
    inicializar_arquivo_historico();
    
    printf("Price Updater melhorado iniciado com configurações:\n");
    printf("- Variação máxima: %.1f%%\n", MAX_VARIACAO_PRECO * 100);
    printf("- Preço mínimo: R$ %.2f\n", MIN_PRECO_ACAO);
    printf("- Preço máximo: R$ %.2f\n", MAX_PRECO_ACAO);
    printf("- Peso transação: %.1f%%\n", PESO_ULTIMA_TRANSACAO * 100);
    printf("- Peso preço atual: %.1f%%\n", PESO_PRECO_ATUAL * 100);
    printf("- Arquivo histórico: %s\n", ARQUIVO_HISTORICO);
    
    // Configurar poll para leitura de pipes
    struct pollfd pfd;
    pfd.fd = pipes->executor_to_price_updater[0]; // Pipe de leitura do executor
    pfd.events = POLLIN;
    
    int contador_snapshot = 0;
    
    while (sistema->sistema_ativo) {
        // Verificar se há notificações de transações
        int poll_result = poll(&pfd, 1, 100); // 100ms timeout
        
        if (poll_result > 0 && (pfd.revents & POLLIN)) {
            // Notificação disponível
            Ordem ordem;
            int resultado;
            int notificacao_recebida = receber_notificacao_transacao(pipes->executor_to_price_updater[0], &ordem, &resultado);
            
            if (notificacao_recebida) {
                printf("PRICE UPDATER: Notificação recebida - Trader %d, Ação %d, Resultado: %s\n", 
                       ordem.trader_id, ordem.acao_id, resultado ? "ACEITA" : "REJEITADA");
                
                if (resultado) { // Ordem aceita
                    // Calcular novo preço usando média ponderada
                    Acao* acao = &sistema->acoes[ordem.acao_id];
                    double preco_anterior = acao->preco_atual;
                    double preco_transacao = ordem.preco;
                    int volume = ordem.quantidade;
                    
                    double novo_preco = calcular_preco_media_ponderada(preco_anterior, preco_transacao, volume);
                    
                    // Validar preço
                    if (validar_preco(novo_preco, preco_anterior)) {
                        // Atualizar preço e estatísticas
                        atualizar_estatisticas_acao(sistema, ordem.acao_id, novo_preco);
                        
                        // Log da atualização
                        log_atualizacao_preco(ordem.acao_id, preco_anterior, novo_preco, "Transação executada");
                        
                        // Enviar atualização para arbitrage monitor
                        enviar_atualizacao_arbitragem(pipes->price_updater_to_arbitrage[1], 
                                                     ordem.acao_id, preco_anterior, novo_preco);
                        
                        atualizacoes_validas++;
                    } else {
                        // Manter preço anterior se inválido
                        printf("PRICE UPDATER: Preço inválido, mantendo preço anterior\n");
                        atualizacoes_rejeitadas++;
                    }
                    
                    total_atualizacoes++;
                }
            }
        }
        
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
                    atualizacoes_validas++;
                } else {
                    atualizacoes_rejeitadas++;
                }
                
                total_atualizacoes++;
            }
            
            // Salvar snapshot a cada 10 atualizações periódicas
            contador_snapshot++;
            if (contador_snapshot >= 10) {
                salvar_historico_precos(sistema);
                contador_snapshot = 0;
                printf("PRICE UPDATER: Snapshot salvo no arquivo de histórico\n");
            }
        }
        
        // Pequena pausa para não sobrecarregar
        usleep(100000); // 100ms
    }
    
    // Estatísticas finais
    printf("=== PRICE UPDATER MELHORADO FINALIZADO ===\n");
    printf("Total de atualizações: %d\n", total_atualizacoes);
    printf("Atualizações válidas: %d (%.1f%%)\n", atualizacoes_validas,
           total_atualizacoes > 0 ? (double)atualizacoes_validas / total_atualizacoes * 100 : 0);
    printf("Atualizações rejeitadas: %d (%.1f%%)\n", atualizacoes_rejeitadas,
           total_atualizacoes > 0 ? (double)atualizacoes_rejeitadas / total_atualizacoes * 100 : 0);
    printf("Notificações recebidas: %d\n", notificacoes_recebidas);
    
    // Salvar snapshot final
    salvar_historico_precos(sistema);
    printf("PRICE UPDATER: Snapshot final salvo\n");
    
    // Desanexar memória compartilhada
    shmdt(sistema);
    shmdt(pipes);
    
    exit(0);
} 