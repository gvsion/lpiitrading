#include "trading_system.h"
#include <math.h>

void inicializar_executor(TradingSystem* sistema) {
    sistema->executor.id = 1;
    strcpy(sistema->executor.nome, "Sistema Executor");
    sistema->executor.saldo_inicial = 0.0;
    sistema->executor.saldo_atual = 0.0;
    sistema->executor.total_ordens = 0;
    sistema->executor.ordens_executadas = 0;
    sistema->executor.ordens_canceladas = 0;
    
    pthread_mutex_init(&sistema->executor.mutex, NULL);
    
    log_evento("Executor inicializado com sucesso");
}

void executar_ordens_pendentes(TradingSystem* sistema) {
    pthread_mutex_lock(&sistema->mutex_geral);
    
    for (int i = 0; i < sistema->num_ordens; i++) {
        Ordem* ordem = &sistema->ordens[i];
        
        if (ordem->status == 0) { // Ordem pendente
            processar_ordem(sistema, i);
        }
    }
    
    pthread_mutex_unlock(&sistema->mutex_geral);
}

void processar_ordem(TradingSystem* sistema, int ordem_id) {
    if (ordem_id < 0 || ordem_id >= sistema->num_ordens) {
        return;
    }
    
    Ordem* ordem = &sistema->ordens[ordem_id];
    Trader* trader = &sistema->traders[ordem->trader_id];
    Acao* acao = &sistema->acoes[ordem->acao_id];
    
    // Verificar se a ordem ainda é válida (preço atual próximo ao preço da ordem)
    double diferenca_preco = fabs(ordem->preco - acao->preco_atual);
    double percentual_diferenca = diferenca_preco / acao->preco_atual;
    
    // Se diferença é maior que 5%, cancelar a ordem
    if (percentual_diferenca > 0.05) {
        cancelar_ordem(sistema, ordem_id);
        return;
    }
    
    pthread_mutex_lock(&trader->mutex);
    pthread_mutex_lock(&acao->mutex);
    
    if (ordem->tipo == 'C') { // Ordem de compra
        double custo_total = ordem->preco * ordem->quantidade;
        
        // Verificar se o trader tem saldo suficiente
        if (trader->saldo >= custo_total) {
            // Executar a compra
            trader->saldo -= custo_total;
            trader->acoes_possuidas[ordem->acao_id] += ordem->quantidade;
            acao->volume_negociado += ordem->quantidade;
            
            ordem->status = 1; // Executada
            sistema->executor.ordens_executadas++;
            
            printf("EXECUTADA: Trader %d comprou %d ações de %s a R$ %.2f\n", 
                   ordem->trader_id, ordem->quantidade, acao->nome, ordem->preco);
            
            log_evento("Ordem de compra executada");
        } else {
            // Saldo insuficiente
            cancelar_ordem(sistema, ordem_id);
            printf("CANCELADA: Trader %d não tem saldo suficiente para comprar %d ações de %s\n", 
                   ordem->trader_id, ordem->quantidade, acao->nome);
        }
    } else if (ordem->tipo == 'V') { // Ordem de venda
        // Verificar se o trader possui ações suficientes
        if (trader->acoes_possuidas[ordem->acao_id] >= ordem->quantidade) {
            double valor_recebido = ordem->preco * ordem->quantidade;
            
            // Executar a venda
            trader->saldo += valor_recebido;
            trader->acoes_possuidas[ordem->acao_id] -= ordem->quantidade;
            acao->volume_negociado += ordem->quantidade;
            
            ordem->status = 1; // Executada
            sistema->executor.ordens_executadas++;
            
            printf("EXECUTADA: Trader %d vendeu %d ações de %s a R$ %.2f\n", 
                   ordem->trader_id, ordem->quantidade, acao->nome, ordem->preco);
            
            log_evento("Ordem de venda executada");
        } else {
            // Ações insuficientes
            cancelar_ordem(sistema, ordem_id);
            printf("CANCELADA: Trader %d não possui ações suficientes para vender %d ações de %s\n", 
                   ordem->trader_id, ordem->quantidade, acao->nome);
        }
    }
    
    pthread_mutex_unlock(&acao->mutex);
    pthread_mutex_unlock(&trader->mutex);
}

void cancelar_ordem(TradingSystem* sistema, int ordem_id) {
    if (ordem_id < 0 || ordem_id >= sistema->num_ordens) {
        return;
    }
    
    Ordem* ordem = &sistema->ordens[ordem_id];
    ordem->status = 2; // Cancelada
    sistema->executor.ordens_canceladas++;
    
    printf("CANCELADA: Ordem %d do trader %d foi cancelada\n", ordem_id, ordem->trader_id);
    log_evento("Ordem cancelada");
}

int criar_ordem(TradingSystem* sistema, int trader_id, int acao_id, char tipo, double preco, int quantidade) {
    if (trader_id < 0 || trader_id >= sistema->num_traders || 
        acao_id < 0 || acao_id >= sistema->num_acoes ||
        sistema->num_ordens >= MAX_ORDENS) {
        return -1;
    }
    
    pthread_mutex_lock(&sistema->mutex_geral);
    
    Ordem* nova_ordem = &sistema->ordens[sistema->num_ordens];
    nova_ordem->id = sistema->num_ordens;
    nova_ordem->trader_id = trader_id;
    nova_ordem->acao_id = acao_id;
    nova_ordem->tipo = tipo;
    nova_ordem->preco = preco;
    nova_ordem->quantidade = quantidade;
    nova_ordem->timestamp = time(NULL);
    nova_ordem->status = 0; // Pendente
    
    sistema->num_ordens++;
    sistema->executor.total_ordens++;
    
    printf("NOVA ORDEM: Trader %d %s %d ações de %s a R$ %.2f\n", 
           trader_id, (tipo == 'C' ? "compra" : "vende"), quantidade, 
           sistema->acoes[acao_id].nome, preco);
    
    pthread_mutex_unlock(&sistema->mutex_geral);
    
    // Sinalizar que há novas ordens para processar
    sem_post(&sistema->sem_ordens);
    
    return nova_ordem->id;
}

void imprimir_estado_executor(TradingSystem* sistema) {
    printf("\n=== ESTADO DO EXECUTOR ===\n");
    printf("Total de ordens: %d\n", sistema->executor.total_ordens);
    printf("Ordens executadas: %d\n", sistema->executor.ordens_executadas);
    printf("Ordens canceladas: %d\n", sistema->executor.ordens_canceladas);
    printf("Taxa de execução: %.2f%%\n", 
           (double)sistema->executor.ordens_executadas / sistema->executor.total_ordens * 100);
    printf("\n");
}

void imprimir_ordens(TradingSystem* sistema) {
    printf("\n=== ORDENS NO SISTEMA ===\n");
    for (int i = 0; i < sistema->num_ordens; i++) {
        Ordem* ordem = &sistema->ordens[i];
        char status_str[20];
        
        switch (ordem->status) {
            case 0: strcpy(status_str, "PENDENTE"); break;
            case 1: strcpy(status_str, "EXECUTADA"); break;
            case 2: strcpy(status_str, "CANCELADA"); break;
            default: strcpy(status_str, "DESCONHECIDO"); break;
        }
        
        printf("Ordem %d: Trader %d %s %d ações de %s a R$ %.2f - %s\n", 
               ordem->id, ordem->trader_id, 
               (ordem->tipo == 'C' ? "COMPRA" : "VENDA"), 
               ordem->quantidade, sistema->acoes[ordem->acao_id].nome, 
               ordem->preco, status_str);
    }
    printf("\n");
}

// Função para verificar se há oportunidades de execução
int verificar_oportunidades_execucao(TradingSystem* sistema) {
    int oportunidades = 0;
    
    for (int i = 0; i < sistema->num_ordens; i++) {
        Ordem* ordem = &sistema->ordens[i];
        
        if (ordem->status == 0) { // Ordem pendente
            Acao* acao = &sistema->acoes[ordem->acao_id];
            double diferenca_preco = fabs(ordem->preco - acao->preco_atual);
            double percentual_diferenca = diferenca_preco / acao->preco_atual;
            
            // Se preço está próximo (diferença menor que 2%), é uma oportunidade
            if (percentual_diferenca <= 0.02) {
                oportunidades++;
            }
        }
    }
    
    return oportunidades;
}

// Função para calcular estatísticas de execução
void calcular_estatisticas_execucao(TradingSystem* sistema) {
    double taxa_execucao = 0.0;
    double taxa_cancelamento = 0.0;
    
    if (sistema->executor.total_ordens > 0) {
        taxa_execucao = (double)sistema->executor.ordens_executadas / sistema->executor.total_ordens * 100;
        taxa_cancelamento = (double)sistema->executor.ordens_canceladas / sistema->executor.total_ordens * 100;
    }
    
    printf("=== ESTATÍSTICAS DE EXECUÇÃO ===\n");
    printf("Taxa de execução: %.2f%%\n", taxa_execucao);
    printf("Taxa de cancelamento: %.2f%%\n", taxa_cancelamento);
    printf("Oportunidades de execução: %d\n", verificar_oportunidades_execucao(sistema));
    printf("\n");
} 