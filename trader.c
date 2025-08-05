#include "trading_system.h"

// Estratégias de trading
typedef enum {
    ESTRATEGIA_CONSERVADORA,
    ESTRATEGIA_AGRESSIVA,
    ESTRATEGIA_MOMENTUM,
    ESTRATEGIA_MEAN_REVERSION,
    ESTRATEGIA_ARBITRAGEM,
    ESTRATEGIA_ALEATORIA
} EstrategiaTrader;

// Estrutura para armazenar dados específicos de cada trader
typedef struct {
    int trader_id;
    EstrategiaTrader estrategia;
    double limite_compra;
    double limite_venda;
    int acao_preferida;
    int frequencia_operacao;
    time_t ultima_operacao;
} DadosTrader;

static DadosTrader dados_traders[MAX_TRADERS];

void inicializar_traders(TradingSystem* sistema) {
    char nomes[MAX_TRADERS][MAX_NOME] = {
        "Trader Conservador",
        "Trader Agressivo", 
        "Trader Momentum",
        "Trader Mean Reversion",
        "Trader Arbitragem",
        "Trader Aleatório"
    };
    
    for (int i = 0; i < MAX_TRADERS; i++) {
        sistema->traders[i].id = i;
        strcpy(sistema->traders[i].nome, nomes[i]);
        sistema->traders[i].saldo = 100000.0; // Saldo inicial de 100k
        
        // Inicializar posições em ações
        for (int j = 0; j < MAX_ACOES; j++) {
            sistema->traders[i].acoes_possuidas[j] = 0;
        }
        
        // Inicializar mutex
        pthread_mutex_init(&sistema->traders[i].mutex, NULL);
        
        // Configurar dados específicos do trader
        dados_traders[i].trader_id = i;
        dados_traders[i].estrategia = i;
        dados_traders[i].limite_compra = 0.95; // 95% do preço atual
        dados_traders[i].limite_venda = 1.05;  // 105% do preço atual
        dados_traders[i].acao_preferida = i % MAX_ACOES;
        dados_traders[i].frequencia_operacao = 5 + (i * 2); // 5-15 segundos
        dados_traders[i].ultima_operacao = 0;
    }
    
    sistema->num_traders = MAX_TRADERS;
    log_evento("Traders inicializados com sucesso");
}

void executar_estrategia_trader(TradingSystem* sistema, int trader_id) {
    if (trader_id < 0 || trader_id >= MAX_TRADERS) {
        return;
    }
    
    DadosTrader* dados = &dados_traders[trader_id];
    Trader* trader = &sistema->traders[trader_id];
    time_t agora = time(NULL);
    
    // Verificar se é hora de operar
    if (agora - dados->ultima_operacao < dados->frequencia_operacao) {
        return;
    }
    
    pthread_mutex_lock(&trader->mutex);
    
    switch (dados->estrategia) {
        case ESTRATEGIA_CONSERVADORA:
            executar_estrategia_conservadora(sistema, trader_id);
            break;
        case ESTRATEGIA_AGRESSIVA:
            executar_estrategia_agressiva(sistema, trader_id);
            break;
        case ESTRATEGIA_MOMENTUM:
            executar_estrategia_momentum(sistema, trader_id);
            break;
        case ESTRATEGIA_MEAN_REVERSION:
            executar_estrategia_mean_reversion(sistema, trader_id);
            break;
        case ESTRATEGIA_ARBITRAGEM:
            executar_estrategia_arbitragem(sistema, trader_id);
            break;
        case ESTRATEGIA_ALEATORIA:
            executar_estrategia_aleatoria(sistema, trader_id);
            break;
    }
    
    dados->ultima_operacao = agora;
    pthread_mutex_unlock(&trader->mutex);
}

void executar_estrategia_conservadora(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    DadosTrader* dados = &dados_traders[trader_id];
    
    // Estratégia conservadora: compra quando preço está baixo, vende quando está alto
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        double preco_atual = acao->preco_atual;
        
        // Comprar se preço caiu mais de 5%
        if (preco_atual < acao->preco_anterior * 0.95 && trader->saldo > preco_atual * 10) {
            int quantidade = 10;
            criar_ordem(sistema, trader_id, i, 'C', preco_atual, quantidade);
            printf("Trader %d (Conservador): Comprou %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
        
        // Vender se preço subiu mais de 5% e possui ações
        if (preco_atual > acao->preco_anterior * 1.05 && trader->acoes_possuidas[i] > 0) {
            int quantidade = trader->acoes_possuidas[i] > 10 ? 10 : trader->acoes_possuidas[i];
            criar_ordem(sistema, trader_id, i, 'V', preco_atual, quantidade);
            printf("Trader %d (Conservador): Vendeu %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
    }
}

void executar_estrategia_agressiva(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Estratégia agressiva: opera com volumes maiores e frequência maior
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        double preco_atual = acao->preco_atual;
        
        // Comprar se há tendência de alta
        if (acao->variacao > 0.02 && trader->saldo > preco_atual * 50) {
            int quantidade = 50;
            criar_ordem(sistema, trader_id, i, 'C', preco_atual, quantidade);
            printf("Trader %d (Agressivo): Comprou %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
        
        // Vender se há tendência de baixa
        if (acao->variacao < -0.02 && trader->acoes_possuidas[i] > 0) {
            int quantidade = trader->acoes_possuidas[i] > 50 ? 50 : trader->acoes_possuidas[i];
            criar_ordem(sistema, trader_id, i, 'V', preco_atual, quantidade);
            printf("Trader %d (Agressivo): Vendeu %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
    }
}

void executar_estrategia_momentum(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Estratégia momentum: segue a tendência
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        double preco_atual = acao->preco_atual;
        
        // Comprar se momentum é positivo
        if (acao->variacao > 0.01 && trader->saldo > preco_atual * 20) {
            int quantidade = 20;
            criar_ordem(sistema, trader_id, i, 'C', preco_atual, quantidade);
            printf("Trader %d (Momentum): Comprou %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
        
        // Vender se momentum é negativo
        if (acao->variacao < -0.01 && trader->acoes_possuidas[i] > 0) {
            int quantidade = trader->acoes_possuidas[i] > 20 ? 20 : trader->acoes_possuidas[i];
            criar_ordem(sistema, trader_id, i, 'V', preco_atual, quantidade);
            printf("Trader %d (Momentum): Vendeu %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
    }
}

void executar_estrategia_mean_reversion(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Estratégia mean reversion: acredita que preços voltam à média
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        double preco_atual = acao->preco_atual;
        
        // Comprar se preço está muito baixo (reversão esperada)
        if (preco_atual < acao->preco_anterior * 0.90 && trader->saldo > preco_atual * 15) {
            int quantidade = 15;
            criar_ordem(sistema, trader_id, i, 'C', preco_atual, quantidade);
            printf("Trader %d (Mean Reversion): Comprou %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
        
        // Vender se preço está muito alto (reversão esperada)
        if (preco_atual > acao->preco_anterior * 1.10 && trader->acoes_possuidas[i] > 0) {
            int quantidade = trader->acoes_possuidas[i] > 15 ? 15 : trader->acoes_possuidas[i];
            criar_ordem(sistema, trader_id, i, 'V', preco_atual, quantidade);
            printf("Trader %d (Mean Reversion): Vendeu %d ações de %s a %.2f\n", 
                   trader_id, quantidade, acao->nome, preco_atual);
        }
    }
}

void executar_estrategia_arbitragem(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Estratégia arbitragem: procura diferenças de preço entre ações similares
    for (int i = 0; i < sistema->num_acoes - 1; i++) {
        for (int j = i + 1; j < sistema->num_acoes; j++) {
            Acao* acao1 = &sistema->acoes[i];
            Acao* acao2 = &sistema->acoes[j];
            
            double diferenca = fabs(acao1->preco_atual - acao2->preco_atual);
            double media = (acao1->preco_atual + acao2->preco_atual) / 2.0;
            double percentual_diferenca = diferenca / media;
            
            // Se diferença é maior que 3%, há oportunidade de arbitragem
            if (percentual_diferenca > 0.03) {
                if (acao1->preco_atual < acao2->preco_atual && trader->saldo > acao1->preco_atual * 10) {
                    // Comprar a mais barata
                    criar_ordem(sistema, trader_id, i, 'C', acao1->preco_atual, 10);
                    printf("Trader %d (Arbitragem): Comprou %s (mais barata) a %.2f\n", 
                           trader_id, acao1->nome, acao1->preco_atual);
                }
            }
        }
    }
}

void executar_estrategia_aleatoria(TradingSystem* sistema, int trader_id) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Estratégia aleatória: toma decisões baseadas em probabilidade
    int acao_aleatoria = rand() % sistema->num_acoes;
    Acao* acao = &sistema->acoes[acao_aleatoria];
    double preco_atual = acao->preco_atual;
    
    int decisao = rand() % 100;
    
    if (decisao < 30 && trader->saldo > preco_atual * 5) {
        // 30% de chance de comprar
        int quantidade = 5;
        criar_ordem(sistema, trader_id, acao_aleatoria, 'C', preco_atual, quantidade);
        printf("Trader %d (Aleatório): Comprou %d ações de %s a %.2f\n", 
               trader_id, quantidade, acao->nome, preco_atual);
    } else if (decisao > 70 && trader->acoes_possuidas[acao_aleatoria] > 0) {
        // 30% de chance de vender
        int quantidade = trader->acoes_possuidas[acao_aleatoria] > 5 ? 5 : trader->acoes_possuidas[acao_aleatoria];
        criar_ordem(sistema, trader_id, acao_aleatoria, 'V', preco_atual, quantidade);
        printf("Trader %d (Aleatório): Vendeu %d ações de %s a %.2f\n", 
               trader_id, quantidade, acao->nome, preco_atual);
    }
}

void imprimir_estado_traders(TradingSystem* sistema) {
    printf("\n=== ESTADO DOS TRADERS ===\n");
    for (int i = 0; i < sistema->num_traders; i++) {
        Trader* trader = &sistema->traders[i];
        printf("Trader %d (%s):\n", trader->id, trader->nome);
        printf("  Saldo: R$ %.2f\n", trader->saldo);
        printf("  Ações possuídas:\n");
        
        for (int j = 0; j < sistema->num_acoes; j++) {
            if (trader->acoes_possuidas[j] > 0) {
                printf("    %s: %d ações\n", sistema->acoes[j].nome, trader->acoes_possuidas[j]);
            }
        }
        printf("\n");
    }
}

// Funções auxiliares (não declaradas no header)
void executar_estrategia_conservadora(TradingSystem* sistema, int trader_id);
void executar_estrategia_agressiva(TradingSystem* sistema, int trader_id);
void executar_estrategia_momentum(TradingSystem* sistema, int trader_id);
void executar_estrategia_mean_reversion(TradingSystem* sistema, int trader_id);
void executar_estrategia_arbitragem(TradingSystem* sistema, int trader_id);
void executar_estrategia_aleatoria(TradingSystem* sistema, int trader_id); 