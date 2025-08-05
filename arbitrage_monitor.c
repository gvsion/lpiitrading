#include "trading_system.h"
#include <math.h>

// Estrutura para armazenar oportunidades de arbitragem
typedef struct {
    int acao1_id;
    int acao2_id;
    double diferenca_preco;
    double percentual_diferenca;
    time_t timestamp;
    int ativa;
} OportunidadeArbitragem;

// Estrutura para armazenar alertas de mercado
typedef struct {
    char tipo[20];
    char descricao[100];
    double valor;
    time_t timestamp;
    int prioridade; // 1: baixa, 2: média, 3: alta
} AlertaMercado;

#define MAX_OPORTUNIDADES 50
#define MAX_ALERTAS 100

static OportunidadeArbitragem oportunidades[MAX_OPORTUNIDADES];
static AlertaMercado alertas[MAX_ALERTAS];
static int num_oportunidades = 0;
static int num_alertas = 0;

void monitorar_arbitragem(TradingSystem* sistema) {
    // Limpar oportunidades antigas (mais de 60 segundos)
    time_t agora = time(NULL);
    for (int i = 0; i < num_oportunidades; i++) {
        if (agora - oportunidades[i].timestamp > 60) {
            oportunidades[i].ativa = 0;
        }
    }
    
    // Procurar novas oportunidades
    for (int i = 0; i < sistema->num_acoes - 1; i++) {
        for (int j = i + 1; j < sistema->num_acoes; j++) {
            Acao* acao1 = &sistema->acoes[i];
            Acao* acao2 = &sistema->acoes[j];
            
            double diferenca = fabs(acao1->preco_atual - acao2->preco_atual);
            double media = (acao1->preco_atual + acao2->preco_atual) / 2.0;
            double percentual_diferenca = diferenca / media;
            
            // Se diferença é maior que 2%, é uma oportunidade
            if (percentual_diferenca > 0.02) {
                registrar_oportunidade_arbitragem(i, j, diferenca, percentual_diferenca);
            }
        }
    }
    
    // Verificar outras condições de mercado
    detectar_arbitragem(sistema);
    verificar_condicoes_mercado(sistema);
}

void detectar_arbitragem(TradingSystem* sistema) {
    // Detectar arbitragem entre diferentes ações do mesmo setor
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        
        // Verificar se preço está muito acima da média histórica
        double preco_atual = acao->preco_atual;
        double preco_anterior = acao->preco_anterior;
        double variacao = acao->variacao;
        
        // Alerta para variações extremas
        if (fabs(variacao) > 0.10) { // Variação maior que 10%
            criar_alerta("VARIAÇÃO EXTREMA", 
                        "Variação muito alta detectada", 
                        variacao * 100, 3);
        }
        
        // Alerta para preços muito baixos (oportunidade de compra)
        if (preco_atual < preco_anterior * 0.85) {
            criar_alerta("OPORTUNIDADE COMPRA", 
                        "Preço muito baixo detectado", 
                        preco_atual, 2);
        }
        
        // Alerta para preços muito altos (oportunidade de venda)
        if (preco_atual > preco_anterior * 1.15) {
            criar_alerta("OPORTUNIDADE VENDA", 
                        "Preço muito alto detectado", 
                        preco_atual, 2);
        }
    }
}

void registrar_oportunidade_arbitragem(int acao1_id, int acao2_id, double diferenca, double percentual) {
    if (num_oportunidades >= MAX_OPORTUNIDADES) {
        // Remover a oportunidade mais antiga
        for (int i = 0; i < num_oportunidades - 1; i++) {
            oportunidades[i] = oportunidades[i + 1];
        }
        num_oportunidades--;
    }
    
    OportunidadeArbitragem* nova = &oportunidades[num_oportunidades];
    nova->acao1_id = acao1_id;
    nova->acao2_id = acao2_id;
    nova->diferenca_preco = diferenca;
    nova->percentual_diferenca = percentual;
    nova->timestamp = time(NULL);
    nova->ativa = 1;
    
    num_oportunidades++;
    
    printf("OPORTUNIDADE DE ARBITRAGEM: Ações %d e %d com diferença de %.2f%%\n", 
           acao1_id, acao2_id, percentual * 100);
}

void verificar_condicoes_mercado(TradingSystem* sistema) {
    // Verificar volume de negociação
    int total_volume = 0;
    for (int i = 0; i < sistema->num_acoes; i++) {
        total_volume += sistema->acoes[i].volume_negociado;
    }
    
    if (total_volume > 1000) {
        criar_alerta("ALTO VOLUME", 
                    "Volume de negociação muito alto", 
                    total_volume, 2);
    }
    
    // Verificar número de ordens pendentes
    int ordens_pendentes = 0;
    for (int i = 0; i < sistema->num_ordens; i++) {
        if (sistema->ordens[i].status == 0) {
            ordens_pendentes++;
        }
    }
    
    if (ordens_pendentes > 50) {
        criar_alerta("MUITAS ORDENS", 
                    "Muitas ordens pendentes no sistema", 
                    ordens_pendentes, 1);
    }
    
    // Verificar saldo dos traders
    for (int i = 0; i < sistema->num_traders; i++) {
        Trader* trader = &sistema->traders[i];
        if (trader->saldo < 1000) {
            criar_alerta("SALDO BAIXO", 
                        "Trader com saldo muito baixo", 
                        trader->saldo, 2);
        }
    }
}

void criar_alerta(const char* tipo, const char* descricao, double valor, int prioridade) {
    if (num_alertas >= MAX_ALERTAS) {
        // Remover o alerta mais antigo
        for (int i = 0; i < num_alertas - 1; i++) {
            alertas[i] = alertas[i + 1];
        }
        num_alertas--;
    }
    
    AlertaMercado* novo = &alertas[num_alertas];
    strcpy(novo->tipo, tipo);
    strcpy(novo->descricao, descricao);
    novo->valor = valor;
    novo->timestamp = time(NULL);
    novo->prioridade = prioridade;
    
    num_alertas++;
    
    char prioridade_str[15];
    switch (prioridade) {
        case 1: strcpy(prioridade_str, "BAIXA"); break;
        case 2: strcpy(prioridade_str, "MÉDIA"); break;
        case 3: strcpy(prioridade_str, "ALTA"); break;
        default: strcpy(prioridade_str, "DESCONHECIDA"); break;
    }
    
    printf("ALERTA [%s]: %s - %.2f\n", prioridade_str, descricao, valor);
}

void imprimir_oportunidades_arbitragem() {
    printf("\n=== OPORTUNIDADES DE ARBITRAGEM ===\n");
    int oportunidades_ativas = 0;
    
    for (int i = 0; i < num_oportunidades; i++) {
        if (oportunidades[i].ativa) {
            printf("Ações %d e %d: Diferença de %.2f%% (R$ %.2f)\n", 
                   oportunidades[i].acao1_id, oportunidades[i].acao2_id,
                   oportunidades[i].percentual_diferenca * 100,
                   oportunidades[i].diferenca_preco);
            oportunidades_ativas++;
        }
    }
    
    if (oportunidades_ativas == 0) {
        printf("Nenhuma oportunidade de arbitragem ativa no momento.\n");
    }
    printf("\n");
}

void imprimir_alertas() {
    printf("\n=== ALERTAS DE MERCADO ===\n");
    time_t agora = time(NULL);
    
    for (int i = 0; i < num_alertas; i++) {
        AlertaMercado* alerta = &alertas[i];
        
        // Mostrar apenas alertas dos últimos 5 minutos
        if (agora - alerta->timestamp < 300) {
            char prioridade_str[15];
            switch (alerta->prioridade) {
                case 1: strcpy(prioridade_str, "BAIXA"); break;
                case 2: strcpy(prioridade_str, "MÉDIA"); break;
                case 3: strcpy(prioridade_str, "ALTA"); break;
                default: strcpy(prioridade_str, "DESCONHECIDA"); break;
            }
            
            printf("[%s] %s: %s (%.2f)\n", 
                   prioridade_str, alerta->tipo, alerta->descricao, alerta->valor);
        }
    }
    printf("\n");
}

// Função para calcular estatísticas de arbitragem
void calcular_estatisticas_arbitragem(TradingSystem* sistema) {
    int total_oportunidades = 0;
    double maior_diferenca = 0.0;
    int acao_mais_volatil = 0;
    double maior_volatilidade = 0.0;
    
    // Contar oportunidades ativas
    for (int i = 0; i < num_oportunidades; i++) {
        if (oportunidades[i].ativa) {
            total_oportunidades++;
            if (oportunidades[i].percentual_diferenca > maior_diferenca) {
                maior_diferenca = oportunidades[i].percentual_diferenca;
            }
        }
    }
    
    // Encontrar ação mais volátil
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        if (fabs(acao->variacao) > maior_volatilidade) {
            maior_volatilidade = fabs(acao->variacao);
            acao_mais_volatil = i;
        }
    }
    
    printf("=== ESTATÍSTICAS DE ARBITRAGEM ===\n");
    printf("Oportunidades ativas: %d\n", total_oportunidades);
    printf("Maior diferença detectada: %.2f%%\n", maior_diferenca * 100);
    printf("Ação mais volátil: %s (%.2f%%)\n", 
           sistema->acoes[acao_mais_volatil].nome, maior_volatilidade * 100);
    printf("Total de alertas: %d\n", num_alertas);
    printf("\n");
}

// Função para simular eventos de mercado que afetam arbitragem
void simular_evento_mercado(TradingSystem* sistema) {
    int tipo_evento = rand() % 4;
    
    switch (tipo_evento) {
        case 0: // Notícia positiva
            {
                int acao = rand() % sistema->num_acoes;
                double impacto = (rand() % 100 + 50) / 1000.0; // +5% a +15%
                double novo_preco = sistema->acoes[acao].preco_atual * (1.0 + impacto);
                
                printf("EVENTO: Notícia positiva para %s (+%.2f%%)\n", 
                       sistema->acoes[acao].nome, impacto * 100);
                
                // Atualizar preço (usar função do price_updater.c)
                // atualizar_preco_acao(sistema, acao, novo_preco);
            }
            break;
            
        case 1: // Notícia negativa
            {
                int acao = rand() % sistema->num_acoes;
                double impacto = -(rand() % 100 + 50) / 1000.0; // -5% a -15%
                double novo_preco = sistema->acoes[acao].preco_atual * (1.0 + impacto);
                
                if (novo_preco < 1.0) novo_preco = 1.0;
                
                printf("EVENTO: Notícia negativa para %s (%.2f%%)\n", 
                       sistema->acoes[acao].nome, impacto * 100);
                
                // Atualizar preço (usar função do price_updater.c)
                // atualizar_preco_acao(sistema, acao, novo_preco);
            }
            break;
            
        case 2: // Alta volatilidade
            printf("EVENTO: Período de alta volatilidade no mercado\n");
            criar_alerta("ALTA VOLATILIDADE", 
                        "Período de alta volatilidade detectado", 
                        0.0, 3);
            break;
            
        case 3: // Baixa liquidez
            printf("EVENTO: Período de baixa liquidez no mercado\n");
            criar_alerta("BAIXA LIQUIDEZ", 
                        "Período de baixa liquidez detectado", 
                        0.0, 2);
            break;
    }
}

// Função para verificar se há oportunidades de arbitragem estatística
void verificar_arbitragem_estatistica(TradingSystem* sistema) {
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        
        // Verificar se preço está muito desviado da média
        double preco_atual = acao->preco_atual;
        double preco_anterior = acao->preco_anterior;
        double variacao = acao->variacao;
        
        // Se variação é muito alta, pode ser uma oportunidade
        if (fabs(variacao) > 0.05) {
            printf("OPORTUNIDADE ESTATÍSTICA: %s com variação de %.2f%%\n", 
                   acao->nome, variacao * 100);
        }
        
        // Verificar se preço está muito baixo comparado ao histórico
        if (preco_atual < preco_anterior * 0.90) {
            printf("OPORTUNIDADE DE COMPRA: %s com preço muito baixo\n", acao->nome);
        }
        
        // Verificar se preço está muito alto comparado ao histórico
        if (preco_atual > preco_anterior * 1.10) {
            printf("OPORTUNIDADE DE VENDA: %s com preço muito alto\n", acao->nome);
        }
    }
} 