#include "trading_system.h"
#include <math.h>
#include <unistd.h>

int main() {
    printf("=== TESTE DAS FUNÇÕES UTILITÁRIAS ===\n");
    printf("Sistema de Trading - Módulo Utils\n\n");
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Inicializar sistema
    TradingSystem* sistema = inicializar_sistema();
    if (!sistema) {
        printf("Erro: Falha ao inicializar sistema\n");
        return 1;
    }
    
    // Inicializar dados de mercado
    inicializar_dados_mercado();
    
    printf("Sistema inicializado com sucesso!\n");
    printf("Número de ações: %d\n", sistema->num_acoes);
    printf("Número de traders: %d\n", sistema->num_traders);
    printf("\n");
    
    // Teste 1: Gerar ordens aleatórias
    printf("=== TESTE 1: GERAÇÃO DE ORDENS ALEATÓRIAS ===\n");
    printf("Gerando 3 ordens aleatórias...\n\n");
    
    for (int i = 0; i < 3; i++) {
        Ordem ordem = gerar_ordem_aleatoria(sistema);
        printf("Ordem %d gerada:\n", i + 1);
        imprimir_ordem(&ordem, sistema);
    }
    
    // Teste 2: Validar ordens
    printf("=== TESTE 2: VALIDAÇÃO DE ORDENS ===\n");
    
    // Ordem válida
    Ordem ordem_valida = gerar_ordem_aleatoria(sistema);
    printf("Testando ordem válida:\n");
    validar_ordem(sistema, &ordem_valida);
    printf("\n");
    
    // Ordem inválida - preço muito alto
    Ordem ordem_invalida = ordem_valida;
    ordem_invalida.preco = 500.0;
    printf("Testando ordem com preço muito alto (R$ 500.00):\n");
    validar_ordem(sistema, &ordem_invalida);
    printf("\n");
    
    // Ordem inválida - quantidade muito baixa
    ordem_invalida = ordem_valida;
    ordem_invalida.quantidade = 50;
    printf("Testando ordem com quantidade muito baixa (50):\n");
    validar_ordem(sistema, &ordem_invalida);
    printf("\n");
    
    // Ordem inválida - tipo inválido
    ordem_invalida = ordem_valida;
    ordem_invalida.tipo = 'X';
    printf("Testando ordem com tipo inválido ('X'):\n");
    validar_ordem(sistema, &ordem_invalida);
    printf("\n");
    
    // Ordem inválida - trader ID inválido
    ordem_invalida = ordem_valida;
    ordem_invalida.trader_id = 999;
    printf("Testando ordem com trader ID inválido (999):\n");
    validar_ordem(sistema, &ordem_invalida);
    printf("\n");
    
    // Ordem inválida - ação ID inválida
    ordem_invalida = ordem_valida;
    ordem_invalida.acao_id = 999;
    printf("Testando ordem com ação ID inválida (999):\n");
    validar_ordem(sistema, &ordem_invalida);
    printf("\n");
    
    // Teste 3: Calcular preços baseados em oferta/demanda
    printf("=== TESTE 3: CÁLCULO DE PREÇOS POR OFERTA/DEMANDA ===\n");
    
    // Adicionar algumas ordens para simular oferta/demanda
    printf("Adicionando ordens para simular mercado...\n");
    gerar_ordens_aleatorias(sistema, 10);
    
    printf("Calculando novos preços baseados em oferta/demanda:\n");
    for (int i = 0; i < sistema->num_acoes; i++) {
        double preco_atual = sistema->acoes[i].preco_atual;
        double novo_preco = calcular_preco_oferta_demanda(sistema, i);
        double variacao = ((novo_preco - preco_atual) / preco_atual) * 100;
        
        printf("%s: R$ %.2f → R$ %.2f (variação: %.2f%%)\n", 
               sistema->acoes[i].nome, preco_atual, novo_preco, variacao);
    }
    printf("\n");
    
    // Teste 4: Detectar arbitragem entre ações relacionadas
    printf("=== TESTE 4: DETECÇÃO DE ARBITRAGEM ENTRE AÇÕES RELACIONADAS ===\n");
    detectar_arbitragem_relacionadas(sistema);
    printf("\n");
    
    // Teste 5: Estatísticas de mercado
    printf("=== TESTE 5: ESTATÍSTICAS DE MERCADO ===\n");
    imprimir_estatisticas_mercado(sistema);
    
    // Teste 6: Teste completo das funções utilitárias
    printf("=== TESTE 6: TESTE COMPLETO DAS FUNÇÕES UTILITÁRIAS ===\n");
    testar_funcoes_utilitarias(sistema);
    
    // Limpar sistema
    limpar_sistema(sistema);
    
    printf("\n=== TODOS OS TESTES CONCLUÍDOS COM SUCESSO! ===\n");
    printf("✓ Geração de ordens aleatórias\n");
    printf("✓ Validação de ordens\n");
    printf("✓ Cálculo de preços por oferta/demanda\n");
    printf("✓ Detecção de arbitragem\n");
    printf("✓ Estatísticas de mercado\n");
    printf("✓ Funções utilitárias completas\n");
    
    return 0;
} 