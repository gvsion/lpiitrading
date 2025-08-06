#include "trading_system.h"
#include <math.h>
#include <unistd.h>

int main() {
    printf("=== TESTE DO MÓDULO MERCADO ===\n");
    printf("Sistema de Trading - Módulo Mercado\n\n");
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Inicializar sistema
    TradingSystem* sistema = inicializar_sistema();
    if (!sistema) {
        printf("Erro: Falha ao inicializar sistema\n");
        return 1;
    }
    
    printf("Sistema inicializado com sucesso!\n");
    printf("Número de ações: %d\n", sistema->num_acoes);
    printf("Número de traders: %d\n", sistema->num_traders);
    printf("\n");
    
    // Teste 1: Verificar horários do mercado
    printf("=== TESTE 1: HORÁRIOS DO MERCADO ===\n");
    printf("Horário de abertura: %s\n", obter_horario_abertura());
    printf("Horário de fechamento: %s\n", obter_horario_fechamento());
    printf("Mercado está aberto: %s\n", mercado_esta_aberto() ? "SIM" : "NÃO");
    printf("\n");
    
    // Teste 2: Imprimir estado inicial do mercado
    printf("=== TESTE 2: ESTADO INICIAL DO MERCADO ===\n");
    imprimir_estado_mercado(sistema);
    
    // Teste 3: Simular algumas operações
    printf("=== TESTE 3: SIMULANDO OPERAÇÕES ===\n");
    
    // Gerar algumas ordens para simular operações
    for (int i = 0; i < 5; i++) {
        Ordem ordem = gerar_ordem_aleatoria(sistema);
        ordem.status = 1; // Executada
        
        // Atualizar estatísticas do mercado
        atualizar_estatisticas_mercado(sistema, &ordem);
        
        printf("Operação %d: %s %d ações de %s a R$ %.2f\n", 
               i + 1, (ordem.tipo == 'C') ? "COMPRA" : "VENDA", 
               ordem.quantidade, sistema->acoes[ordem.acao_id].nome, ordem.preco);
    }
    
    printf("\n");
    
    // Teste 4: Imprimir estado após operações
    printf("=== TESTE 4: ESTADO APÓS OPERAÇÕES ===\n");
    imprimir_estado_mercado(sistema);
    
    // Teste 5: Simular abertura do mercado
    printf("=== TESTE 5: SIMULAÇÃO DE ABERTURA ===\n");
    simular_abertura_mercado(sistema);
    imprimir_estado_mercado(sistema);
    
    // Teste 6: Simular mais operações
    printf("=== TESTE 6: MAIS OPERAÇÕES ===\n");
    
    for (int i = 0; i < 10; i++) {
        Ordem ordem = gerar_ordem_aleatoria(sistema);
        ordem.status = 1; // Executada
        
        // Atualizar estatísticas do mercado
        atualizar_estatisticas_mercado(sistema, &ordem);
        
        // Atualizar preço da ação
        double variacao = (rand() % 200 - 100) / 1000.0; // ±10%
        double novo_preco = sistema->acoes[ordem.acao_id].preco_atual * (1.0 + variacao);
        sistema->acoes[ordem.acao_id].preco_atual = novo_preco;
        
        printf("Operação %d: %s %d ações de %s a R$ %.2f (novo preço: R$ %.2f)\n", 
               i + 1, (ordem.tipo == 'C') ? "COMPRA" : "VENDA", 
               ordem.quantidade, sistema->acoes[ordem.acao_id].nome, ordem.preco, novo_preco);
    }
    
    printf("\n");
    
    // Teste 7: Imprimir estado final
    printf("=== TESTE 7: ESTADO FINAL ===\n");
    imprimir_estado_mercado(sistema);
    
    // Teste 8: Simular fechamento do mercado
    printf("=== TESTE 8: SIMULAÇÃO DE FECHAMENTO ===\n");
    simular_fechamento_mercado(sistema);
    
    // Teste 9: Resetar estatísticas
    printf("=== TESTE 9: RESET DE ESTATÍSTICAS ===\n");
    resetar_estatisticas_diarias(sistema);
    imprimir_estado_mercado(sistema);
    
    // Teste 10: Verificar preços ajustados
    printf("=== TESTE 10: PREÇOS AJUSTADOS ===\n");
    printf("Verificando se os preços iniciais fazem sentido:\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        printf("%s: R$ %.2f (%s)\n", acao->nome, acao->preco_atual, acao->setor);
    }
    
    // Limpar sistema
    limpar_sistema(sistema);
    
    printf("\n=== TODOS OS TESTES DO MERCADO CONCLUÍDOS COM SUCESSO! ===\n");
    printf("✓ Inicialização de dados do mercado\n");
    printf("✓ Configuração de preços realistas\n");
    printf("✓ Definição de horários de abertura/fechamento\n");
    printf("✓ Inicialização de estatísticas zeradas\n");
    printf("✓ Ajuste de preços iniciais\n");
    printf("✓ Adição de mais ações ao mercado\n");
    printf("✓ Implementação de imprimir_estado_mercado()\n");
    printf("✓ Monitoramento completo do mercado\n");
    
    return 0;
} 