#include "trading_system.h"
#include <math.h>
#include <unistd.h>

// Estrutura para armazenar dados de oferta/demanda
typedef struct {
    int ordens_compra;
    int ordens_venda;
    double preco_medio_compra;
    double preco_medio_venda;
    int volume_compra;
    int volume_venda;
} OfertaDemanda;

static OfertaDemanda dados_mercado[MAX_ACOES];

// Símbolos das ações brasileiras
static const char* SIMBOLOS_ACOES[] = {
    "PETR4", "VALE3", "ITUB4", "ABEV3", "BBAS3", "BBDC4", "WEGE3", "RENT3", "LREN3", "MGLU3"
};

// Preços médios das ações (baseados em dados reais aproximados)
static const double PRECOS_MEDIOS[] = {
    25.50, 68.30, 32.15, 14.20, 45.80, 15.80, 45.90, 55.40, 18.75, 3.25
};

// Volatilidades das ações (baseadas em dados reais)
static const double VOLATILIDADES[] = {
    0.025, 0.035, 0.020, 0.030, 0.022, 0.028, 0.018, 0.032, 0.040, 0.050
};

// Função para gerar ordens aleatórias realistas
Ordem gerar_ordem_aleatoria(TradingSystem* sistema) {
    Ordem ordem;
    
    // Gerar trader aleatório
    ordem.trader_id = rand() % sistema->num_traders;
    
    // Gerar ação aleatória
    ordem.acao_id = rand() % sistema->num_acoes;
    
    // Gerar tipo de ordem (60% compra, 40% venda)
    ordem.tipo = (rand() % 100 < 60) ? 'C' : 'V';
    
    // Gerar quantidade realista (100-1000 ações)
    ordem.quantidade = 100 + (rand() % 901);
    
    // Gerar preço baseado no preço atual da ação com variação realista
    Acao* acao = &sistema->acoes[ordem.acao_id];
    double preco_atual = acao->preco_atual;
    double variacao = (rand() % 200 - 100) / 1000.0; // ±10%
    ordem.preco = preco_atual * (1.0 + variacao);
    
    // Garantir preço mínimo
    if (ordem.preco < 10.0) ordem.preco = 10.0;
    if (ordem.preco > 200.0) ordem.preco = 200.0;
    
    // Gerar timestamp
    ordem.timestamp = time(NULL);
    
    // Status inicial
    ordem.status = 0; // Pendente
    
    return ordem;
}

// Função para gerar múltiplas ordens aleatórias
void gerar_ordens_aleatorias(TradingSystem* sistema, int num_ordens) {
    printf("Gerando %d ordens aleatórias...\n", num_ordens);
    
    for (int i = 0; i < num_ordens && sistema->num_ordens < MAX_ORDENS; i++) {
        Ordem nova_ordem = gerar_ordem_aleatoria(sistema);
        
        // Validar ordem antes de adicionar
        if (validar_ordem(sistema, &nova_ordem)) {
            // Adicionar ordem ao sistema
            pthread_mutex_lock(&sistema->mutex_geral);
            
            nova_ordem.id = sistema->num_ordens;
            sistema->ordens[sistema->num_ordens] = nova_ordem;
            sistema->num_ordens++;
            sistema->executor.total_ordens++;
            
            pthread_mutex_unlock(&sistema->mutex_geral);
            
            // Imprimir ordem para debug
            imprimir_ordem(&nova_ordem, sistema);
        } else {
            printf("Ordem inválida gerada, descartando...\n");
        }
        
        // Pequena pausa entre ordens
        usleep(100000); // 0.1 segundo
    }
}

// Função para calcular novo preço baseado em oferta/demanda
double calcular_preco_oferta_demanda(TradingSystem* sistema, int acao_id) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return sistema->acoes[acao_id].preco_atual;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    OfertaDemanda* od = &dados_mercado[acao_id];
    
    // Calcular pressão de compra vs venda
    double pressao_compra = 0.0;
    double pressao_venda = 0.0;
    
    // Analisar ordens pendentes
    for (int i = 0; i < sistema->num_ordens; i++) {
        Ordem* ordem = &sistema->ordens[i];
        if (ordem->acao_id == acao_id && ordem->status == 0) {
            if (ordem->tipo == 'C') {
                pressao_compra += ordem->quantidade * (ordem->preco / acao->preco_atual);
                od->ordens_compra++;
                od->volume_compra += ordem->quantidade;
                od->preco_medio_compra += ordem->preco;
            } else {
                pressao_venda += ordem->quantidade * (acao->preco_atual / ordem->preco);
                od->ordens_venda++;
                od->volume_venda += ordem->quantidade;
                od->preco_medio_venda += ordem->preco;
            }
        }
    }
    
    // Calcular médias
    if (od->ordens_compra > 0) {
        od->preco_medio_compra /= od->ordens_compra;
    }
    if (od->ordens_venda > 0) {
        od->preco_medio_venda /= od->ordens_venda;
    }
    
    // Calcular variação baseada na pressão
    double variacao = 0.0;
    
    if (pressao_compra > pressao_venda) {
        // Mais compradores - preço sobe
        double intensidade = (pressao_compra - pressao_venda) / (pressao_compra + pressao_venda);
        variacao = intensidade * 0.05; // Máximo 5% de alta
    } else if (pressao_venda > pressao_compra) {
        // Mais vendedores - preço cai
        double intensidade = (pressao_venda - pressao_compra) / (pressao_compra + pressao_venda);
        variacao = -intensidade * 0.05; // Máximo 5% de baixa
    }
    
    // Adicionar componente aleatório
    double variacao_aleatoria = (rand() % 200 - 100) / 10000.0; // ±1%
    variacao += variacao_aleatoria;
    
    // Calcular novo preço
    double novo_preco = acao->preco_atual * (1.0 + variacao);
    
    // Garantir limites
    if (novo_preco < 10.0) novo_preco = 10.0;
    if (novo_preco > 200.0) novo_preco = 200.0;
    
    return novo_preco;
}

// Função para detectar oportunidades de arbitragem entre ações relacionadas
void detectar_arbitragem_relacionadas(TradingSystem* sistema) {
    printf("\n=== DETECÇÃO DE ARBITRAGEM ENTRE AÇÕES RELACIONADAS ===\n");
    
    // Grupos de ações relacionadas (mesmo setor)
    int grupos[][3] = {
        {0, 1, -1},  // Petrobras e Vale (commodities)
        {2, 3, 4},   // Bancos (Itaú, Bradesco, BB)
        {5, 6, -1},  // Varejo (Magazine Luiza, Lojas Renner)
        {7, 8, -1},  // Varejo (Lojas Renner, Magazine Luiza)
        {9, -1, -1}  // Magazine Luiza
    };
    
    int num_grupos = 5;
    
    for (int g = 0; g < num_grupos; g++) {
        int acao1 = grupos[g][0];
        int acao2 = grupos[g][1];
        int acao3 = grupos[g][2];
        
        if (acao1 >= 0 && acao2 >= 0) {
            Acao* a1 = &sistema->acoes[acao1];
            Acao* a2 = &sistema->acoes[acao2];
            
            // Calcular correlação esperada
            double correlacao_esperada = 0.7; // Ações do mesmo setor devem ter correlação alta
            
            // Calcular correlação atual
            double correlacao_atual = calcular_correlacao(sistema, acao1, acao2);
            
            // Detectar divergência
            if (fabs(correlacao_atual - correlacao_esperada) > 0.3) {
                printf("ARBITRAGEM DETECTADA: %s e %s com correlação %.2f (esperado: %.2f)\n", 
                       a1->nome, a2->nome, correlacao_atual, correlacao_esperada);
                
                // Calcular oportunidade de arbitragem
                double diferenca_preco = fabs(a1->preco_atual - a2->preco_atual);
                double media_preco = (a1->preco_atual + a2->preco_atual) / 2.0;
                double percentual_diferenca = diferenca_preco / media_preco;
                
                if (percentual_diferenca > 0.05) { // 5% de diferença
                    printf("  OPORTUNIDADE: Diferença de %.2f%% entre %s (R$ %.2f) e %s (R$ %.2f)\n",
                           percentual_diferenca * 100, a1->nome, a1->preco_atual, 
                           a2->nome, a2->preco_atual);
                    
                    // Sugerir ação
                    if (a1->preco_atual < a2->preco_atual) {
                        printf("  SUGESTÃO: Comprar %s, vender %s\n", a1->nome, a2->nome);
                    } else {
                        printf("  SUGESTÃO: Comprar %s, vender %s\n", a2->nome, a1->nome);
                    }
                }
            }
        }
        
        // Verificar terceira ação se existir
        if (acao3 >= 0 && acao1 >= 0) {
            Acao* a1 = &sistema->acoes[acao1];
            Acao* a3 = &sistema->acoes[acao3];
            
            double correlacao_atual = calcular_correlacao(sistema, acao1, acao3);
            
            if (fabs(correlacao_atual - 0.7) > 0.3) {
                printf("ARBITRAGEM DETECTADA: %s e %s com correlação %.2f\n", 
                       a1->nome, a3->nome, correlacao_atual);
            }
        }
    }
}

// Função para validar ordem (TAREFA DO ALUNO)
int validar_ordem(TradingSystem* sistema, Ordem* ordem) {
    if (!sistema || !ordem) {
        printf("ERRO: Sistema ou ordem inválidos\n");
        return 0;
    }
    
    // Validar trader
    if (ordem->trader_id < 0 || ordem->trader_id >= sistema->num_traders) {
        printf("ERRO: Trader ID inválido: %d\n", ordem->trader_id);
        return 0;
    }
    
    // Validar ação
    if (ordem->acao_id < 0 || ordem->acao_id >= sistema->num_acoes) {
        printf("ERRO: Ação ID inválida: %d\n", ordem->acao_id);
        return 0;
    }
    
    // Validar tipo de ordem
    if (ordem->tipo != 'C' && ordem->tipo != 'V') {
        printf("ERRO: Tipo de ordem inválido: %c\n", ordem->tipo);
        return 0;
    }
    
    // Validar preço
    if (ordem->preco < 10.0 || ordem->preco > 200.0) {
        printf("ERRO: Preço fora do intervalo válido: R$ %.2f\n", ordem->preco);
        return 0;
    }
    
    // Validar quantidade
    if (ordem->quantidade < 100 || ordem->quantidade > 1000) {
        printf("ERRO: Quantidade fora do intervalo válido: %d\n", ordem->quantidade);
        return 0;
    }
    
    // Validar se trader tem saldo suficiente (para compras)
    if (ordem->tipo == 'C') {
        Trader* trader = &sistema->traders[ordem->trader_id];
        double custo_total = ordem->preco * ordem->quantidade;
        
        if (trader->saldo < custo_total) {
            printf("ERRO: Saldo insuficiente. Necessário: R$ %.2f, Disponível: R$ %.2f\n", 
                   custo_total, trader->saldo);
            return 0;
        }
    }
    
    // Validar se trader tem ações suficientes (para vendas)
    if (ordem->tipo == 'V') {
        Trader* trader = &sistema->traders[ordem->trader_id];
        
        if (trader->acoes_possuidas[ordem->acao_id] < ordem->quantidade) {
            printf("ERRO: Ações insuficientes. Necessário: %d, Disponível: %d\n", 
                   ordem->quantidade, trader->acoes_possuidas[ordem->acao_id]);
            return 0;
        }
    }
    
    // Validar timestamp
    if (ordem->timestamp <= 0) {
        printf("ERRO: Timestamp inválido\n");
        return 0;
    }
    
    printf("✓ Ordem válida\n");
    return 1;
}

// Função para imprimir ordem (TAREFA DO ALUNO)
void imprimir_ordem(Ordem* ordem, TradingSystem* sistema) {
    if (!ordem || !sistema) {
        printf("ERRO: Ordem ou sistema inválidos\n");
        return;
    }
    
    printf("=== ORDEM #%d ===\n", ordem->id);
    printf("Trader: %s (ID: %d)\n", sistema->traders[ordem->trader_id].nome, ordem->trader_id);
    printf("Ação: %s (ID: %d)\n", sistema->acoes[ordem->acao_id].nome, ordem->acao_id);
    printf("Tipo: %s\n", (ordem->tipo == 'C') ? "COMPRA" : "VENDA");
    printf("Quantidade: %d ações\n", ordem->quantidade);
    printf("Preço: R$ %.2f\n", ordem->preco);
    printf("Valor Total: R$ %.2f\n", ordem->preco * ordem->quantidade);
    
    // Status da ordem
    char status_str[20];
    switch (ordem->status) {
        case 0: strcpy(status_str, "PENDENTE"); break;
        case 1: strcpy(status_str, "EXECUTADA"); break;
        case 2: strcpy(status_str, "CANCELADA"); break;
        default: strcpy(status_str, "DESCONHECIDO"); break;
    }
    printf("Status: %s\n", status_str);
    
    // Timestamp
    char* timestamp = ctime(&ordem->timestamp);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remover \n
    printf("Timestamp: %s\n", timestamp);
    
    // Informações adicionais do trader
    Trader* trader = &sistema->traders[ordem->trader_id];
    printf("Saldo do Trader: R$ %.2f\n", trader->saldo);
    printf("Ações possuídas de %s: %d\n", 
           sistema->acoes[ordem->acao_id].nome, 
           trader->acoes_possuidas[ordem->acao_id]);
    
    printf("================\n\n");
}

// Função para testar as funções utilitárias
void testar_funcoes_utilitarias(TradingSystem* sistema) {
    printf("\n=== TESTE DAS FUNÇÕES UTILITÁRIAS ===\n");
    
    // Teste 1: Gerar ordens aleatórias
    printf("1. Testando geração de ordens aleatórias...\n");
    gerar_ordens_aleatorias(sistema, 5);
    
    // Teste 2: Calcular preços baseados em oferta/demanda
    printf("\n2. Testando cálculo de preços por oferta/demanda...\n");
    for (int i = 0; i < sistema->num_acoes; i++) {
        double preco_atual = sistema->acoes[i].preco_atual;
        double novo_preco = calcular_preco_oferta_demanda(sistema, i);
        printf("%s: R$ %.2f → R$ %.2f (variação: %.2f%%)\n", 
               sistema->acoes[i].nome, preco_atual, novo_preco, 
               ((novo_preco - preco_atual) / preco_atual) * 100);
    }
    
    // Teste 3: Detectar arbitragem entre ações relacionadas
    printf("\n3. Testando detecção de arbitragem...\n");
    detectar_arbitragem_relacionadas(sistema);
    
    // Teste 4: Validar ordens
    printf("\n4. Testando validação de ordens...\n");
    
    // Ordem válida
    Ordem ordem_valida = gerar_ordem_aleatoria(sistema);
    validar_ordem(sistema, &ordem_valida);
    
    // Ordem inválida (preço muito alto)
    Ordem ordem_invalida = ordem_valida;
    ordem_invalida.preco = 500.0;
    validar_ordem(sistema, &ordem_invalida);
    
    // Ordem inválida (quantidade muito baixa)
    ordem_invalida = ordem_valida;
    ordem_invalida.quantidade = 50;
    validar_ordem(sistema, &ordem_invalida);
    
    // Ordem inválida (tipo inválido)
    ordem_invalida = ordem_valida;
    ordem_invalida.tipo = 'X';
    validar_ordem(sistema, &ordem_invalida);
    
    printf("\n=== TESTES CONCLUÍDOS ===\n");
}

// Função para inicializar dados de mercado (utils)
void inicializar_dados_mercado_utils() {
    for (int i = 0; i < MAX_ACOES; i++) {
        dados_mercado[i].ordens_compra = 0;
        dados_mercado[i].ordens_venda = 0;
        dados_mercado[i].preco_medio_compra = 0.0;
        dados_mercado[i].preco_medio_venda = 0.0;
        dados_mercado[i].volume_compra = 0;
        dados_mercado[i].volume_venda = 0;
    }
}

// Função para imprimir estatísticas de mercado
void imprimir_estatisticas_mercado(TradingSystem* sistema) {
    printf("\n=== ESTATÍSTICAS DE MERCADO ===\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        OfertaDemanda* od = &dados_mercado[i];
        Acao* acao = &sistema->acoes[i];
        
        printf("%s:\n", acao->nome);
        printf("  Preço atual: R$ %.2f\n", acao->preco_atual);
        printf("  Ordens de compra: %d (volume: %d)\n", od->ordens_compra, od->volume_compra);
        printf("  Ordens de venda: %d (volume: %d)\n", od->ordens_venda, od->volume_venda);
        
        if (od->ordens_compra > 0) {
            printf("  Preço médio compra: R$ %.2f\n", od->preco_medio_compra);
        }
        if (od->ordens_venda > 0) {
            printf("  Preço médio venda: R$ %.2f\n", od->preco_medio_venda);
        }
        
        double spread = 0.0;
        if (od->ordens_compra > 0 && od->ordens_venda > 0) {
            spread = ((od->preco_medio_venda - od->preco_medio_compra) / od->preco_medio_compra) * 100;
            printf("  Spread: %.2f%%\n", spread);
        }
        
        printf("\n");
    }
} 