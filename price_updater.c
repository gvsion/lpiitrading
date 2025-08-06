#include "trading_system.h"
#include <math.h>

// Estrutura para armazenar histórico de preços
typedef struct {
    double precos[100]; // Últimos 100 preços
    int indice;
    int total_precos;
    double preco_medio;
    double volatilidade;
} HistoricoPreco;

static HistoricoPreco historicos[MAX_ACOES];

void inicializar_acoes(TradingSystem* sistema) {
    // Usar a função do módulo mercado para inicializar ações
    inicializar_acoes_mercado(sistema);
}

void atualizar_preco_acao(TradingSystem* sistema, int acao_id, double novo_preco) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    HistoricoPreco* historico = &historicos[acao_id];
    
    pthread_mutex_lock(&acao->mutex);
    
    // Atualizar preços
    acao->preco_anterior = acao->preco_atual;
    acao->preco_atual = novo_preco;
    acao->variacao = (novo_preco - acao->preco_anterior) / acao->preco_anterior;
    
    // Atualizar histórico
    historico->precos[historico->indice] = novo_preco;
    historico->indice = (historico->indice + 1) % 100;
    if (historico->total_precos < 100) {
        historico->total_precos++;
    }
    
    // Calcular preço médio
    double soma = 0.0;
    int total = historico->total_precos;
    for (int i = 0; i < total; i++) {
        soma += historico->precos[i];
    }
    historico->preco_medio = soma / total;
    
    // Calcular volatilidade (desvio padrão)
    double soma_quadrados = 0.0;
    for (int i = 0; i < total; i++) {
        double diferenca = historico->precos[i] - historico->preco_medio;
        soma_quadrados += diferenca * diferenca;
    }
    historico->volatilidade = sqrt(soma_quadrados / total) / historico->preco_medio;
    
    pthread_mutex_unlock(&acao->mutex);
    
    printf("PREÇO ATUALIZADO: %s - R$ %.2f (variação: %.2f%%)\n", 
           acao->nome, novo_preco, acao->variacao * 100);
}

void gerar_atualizacao_preco(TradingSystem* sistema, int acao_id) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    HistoricoPreco* historico = &historicos[acao_id];
    
    pthread_mutex_lock(&acao->mutex);
    
    double preco_atual = acao->preco_atual;
    double volatilidade = historico->volatilidade;
    double preco_medio = historico->preco_medio;
    
    // Gerar variação baseada na volatilidade e tendência
    double variacao_base = (rand() % 200 - 100) / 10000.0; // ±1%
    double variacao_volatilidade = (rand() % 200 - 100) / 10000.0 * volatilidade;
    double variacao_tendencia = (preco_medio - preco_atual) / preco_atual * 0.1; // Tendência para a média
    
    double variacao_total = variacao_base + variacao_volatilidade + variacao_tendencia;
    
    // Limitar variação máxima
    if (variacao_total > 0.05) variacao_total = 0.05;
    if (variacao_total < -0.05) variacao_total = -0.05;
    
    double novo_preco = preco_atual * (1.0 + variacao_total);
    
    // Garantir preço mínimo
    if (novo_preco < 1.0) novo_preco = 1.0;
    
    pthread_mutex_unlock(&acao->mutex);
    
    atualizar_preco_acao(sistema, acao_id, novo_preco);
}

void atualizar_todos_precos(TradingSystem* sistema) {
    for (int i = 0; i < sistema->num_acoes; i++) {
        gerar_atualizacao_preco(sistema, i);
    }
}

void imprimir_estado_acoes(TradingSystem* sistema) {
    printf("\n=== ESTADO DAS AÇÕES ===\n");
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        HistoricoPreco* historico = &historicos[i];
        
        printf("%s:\n", acao->nome);
        printf("  Preço atual: R$ %.2f\n", acao->preco_atual);
        printf("  Variação: %.2f%%\n", acao->variacao * 100);
        printf("  Volume negociado: %d\n", acao->volume_negociado);
        printf("  Preço médio: R$ %.2f\n", historico->preco_medio);
        printf("  Volatilidade: %.2f%%\n", historico->volatilidade * 100);
        printf("\n");
    }
}

// Função para simular notícias que afetam preços
void simular_noticia_mercado(TradingSystem* sistema) {
    int acao_afetada = rand() % sistema->num_acoes;
    double impacto = (rand() % 200 - 100) / 1000.0; // ±10%
    
    Acao* acao = &sistema->acoes[acao_afetada];
    double novo_preco = acao->preco_atual * (1.0 + impacto);
    
    if (novo_preco < 1.0) novo_preco = 1.0;
    
    printf("NOTÍCIA DE MERCADO: %s afetada por notícia (impacto: %.2f%%)\n", 
           acao->nome, impacto * 100);
    
    atualizar_preco_acao(sistema, acao_afetada, novo_preco);
}

// Função para detectar padrões de preço
void detectar_padroes_preco(TradingSystem* sistema) {
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        HistoricoPreco* historico = &historicos[i];
        
        if (historico->total_precos < 10) continue;
        
        // Detectar tendência de alta/baixa
        double preco_recente = historico->precos[(historico->indice - 1 + 100) % 100];
        double preco_antigo = historico->precos[(historico->indice - 10 + 100) % 100];
        double tendencia = (preco_recente - preco_antigo) / preco_antigo;
        
        if (tendencia > 0.05) {
            printf("PADRÃO DETECTADO: %s em tendência de alta (%.2f%%)\n", 
                   acao->nome, tendencia * 100);
        } else if (tendencia < -0.05) {
            printf("PADRÃO DETECTADO: %s em tendência de baixa (%.2f%%)\n", 
                   acao->nome, tendencia * 100);
        }
        
        // Detectar alta volatilidade
        if (historico->volatilidade > 0.05) {
            printf("ALTA VOLATILIDADE: %s com volatilidade de %.2f%%\n", 
                   acao->nome, historico->volatilidade * 100);
        }
    }
}

// Função para calcular correlação entre ações
double calcular_correlacao(TradingSystem* sistema, int acao1, int acao2) {
    if (acao1 < 0 || acao1 >= sistema->num_acoes || 
        acao2 < 0 || acao2 >= sistema->num_acoes) {
        return 0.0;
    }
    
    HistoricoPreco* hist1 = &historicos[acao1];
    HistoricoPreco* hist2 = &historicos[acao2];
    
    if (hist1->total_precos < 20 || hist2->total_precos < 20) {
        return 0.0;
    }
    
    double media1 = hist1->preco_medio;
    double media2 = hist2->preco_medio;
    
    double soma_produtos = 0.0;
    double soma_quadrados1 = 0.0;
    double soma_quadrados2 = 0.0;
    
    int n = hist1->total_precos < hist2->total_precos ? hist1->total_precos : hist2->total_precos;
    
    for (int i = 0; i < n; i++) {
        double diff1 = hist1->precos[i] - media1;
        double diff2 = hist2->precos[i] - media2;
        
        soma_produtos += diff1 * diff2;
        soma_quadrados1 += diff1 * diff1;
        soma_quadrados2 += diff2 * diff2;
    }
    
    if (soma_quadrados1 == 0 || soma_quadrados2 == 0) {
        return 0.0;
    }
    
    return soma_produtos / sqrt(soma_quadrados1 * soma_quadrados2);
}

// Função para imprimir correlações entre ações
void imprimir_correlacoes(TradingSystem* sistema) {
    printf("\n=== CORRELAÇÕES ENTRE AÇÕES ===\n");
    for (int i = 0; i < sistema->num_acoes - 1; i++) {
        for (int j = i + 1; j < sistema->num_acoes; j++) {
            double correlacao = calcular_correlacao(sistema, i, j);
            printf("%s vs %s: %.3f\n", 
                   sistema->acoes[i].nome, sistema->acoes[j].nome, correlacao);
        }
    }
    printf("\n");
} 