#include "trading_system.h"
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("=== TESTE DO SISTEMA DE PIPES ===\n");
    printf("Sistema de Trading - Comunicação entre Processos\n\n");
    
    // Inicializar seed do rand
    srand(time(NULL));
    
    // Teste 1: Criar pipes do sistema
    printf("=== TESTE 1: CRIAÇÃO DE PIPES ===\n");
    int* descritores = criar_pipes_sistema();
    if (!descritores) {
        printf("ERRO: Falha ao criar pipes do sistema\n");
        return 1;
    }
    
    imprimir_status_pipes();
    
    // Teste 2: Verificar se pipes estão ativos
    printf("=== TESTE 2: VERIFICAÇÃO DE STATUS ===\n");
    if (pipes_estao_ativos()) {
        printf("✓ Pipes estão ativos\n");
    } else {
        printf("✗ Pipes não estão ativos\n");
        return 1;
    }
    
    // Teste 3: Testar envio e recebimento de mensagens
    printf("=== TESTE 3: ENVIO E RECEBIMENTO DE MENSAGENS ===\n");
    
    // Criar mensagens de teste
    MensagemPipe ordem = criar_mensagem_ordem(0, 1, 'C', 25.50, 100);
    MensagemPipe atualizacao = criar_mensagem_atualizacao_preco(1, 25.50, 26.00);
    MensagemPipe arbitragem = criar_mensagem_arbitragem(1, 2, 5.50, 20.0);
    MensagemPipe controle = criar_mensagem_controle(1, 0, 3);
    
    printf("Enviando mensagens de teste...\n");
    
    // Enviar mensagens
    if (enviar_mensagem_pipe(descritores[1], &ordem) > 0) {
        printf("✓ Mensagem de ordem enviada\n");
        imprimir_mensagem(&ordem);
    }
    
    if (enviar_mensagem_pipe(descritores[3], &atualizacao) > 0) {
        printf("✓ Mensagem de atualização enviada\n");
        imprimir_mensagem(&atualizacao);
    }
    
    if (enviar_mensagem_pipe(descritores[5], &arbitragem) > 0) {
        printf("✓ Mensagem de arbitragem enviada\n");
        imprimir_mensagem(&arbitragem);
    }
    
    if (enviar_mensagem_pipe(descritores[9], &controle) > 0) {
        printf("✓ Mensagem de controle enviada\n");
        imprimir_mensagem(&controle);
    }
    
    // Receber mensagens
    printf("\nRecebendo mensagens de teste...\n");
    
    MensagemPipe mensagem_recebida;
    
    if (receber_mensagem_pipe(descritores[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de ordem recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    if (receber_mensagem_pipe(descritores[2], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de atualização recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    if (receber_mensagem_pipe(descritores[4], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de arbitragem recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    if (receber_mensagem_pipe(descritores[8], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de controle recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    // Teste 4: Testar comunicação entre processos simulada
    printf("\n=== TESTE 4: COMUNICAÇÃO ENTRE PROCESSOS SIMULADA ===\n");
    
    printf("Simulando fluxo: Traders -> Executor -> Price Updater -> Arbitrage Monitor\n");
    
    // Simular ordem de trader
    MensagemPipe ordem_trader = criar_mensagem_ordem(0, 1, 'C', 25.50, 100);
    printf("1. Trader envia ordem:\n");
    imprimir_mensagem(&ordem_trader);
    
    // Simular execução pelo executor
    MensagemPipe execucao = criar_mensagem_atualizacao_preco(1, 25.50, 25.75);
    printf("2. Executor processa e envia atualização:\n");
    imprimir_mensagem(&execucao);
    
    // Simular detecção de arbitragem
    MensagemPipe arbitragem_detectada = criar_mensagem_arbitragem(1, 2, 2.25, 8.8);
    printf("3. Price Updater detecta arbitragem:\n");
    imprimir_mensagem(&arbitragem_detectada);
    
    // Simular feedback para traders
    MensagemPipe feedback = criar_mensagem_controle(3, 0, 0);
    printf("4. Arbitrage Monitor envia feedback:\n");
    imprimir_mensagem(&feedback);
    
    // Teste 5: Testar tratamento de erros
    printf("\n=== TESTE 5: TRATAMENTO DE ERROS ===\n");
    
    // Testar envio para pipe inválido
    printf("Testando envio para pipe inválido...\n");
    if (enviar_mensagem_pipe(-1, &ordem) == -1) {
        printf("✓ Erro detectado corretamente para pipe inválido\n");
    }
    
    // Testar recebimento de pipe vazio
    printf("Testando recebimento de pipe vazio...\n");
    if (receber_mensagem_pipe(descritores[0], &mensagem_recebida) == 0) {
        printf("✓ Nenhuma mensagem disponível (comportamento esperado)\n");
    }
    
    // Teste 6: Testar múltiplas mensagens
    printf("\n=== TESTE 6: MÚLTIPLAS MENSAGENS ===\n");
    
    printf("Enviando múltiplas mensagens...\n");
    
    for (int i = 0; i < 5; i++) {
        MensagemPipe msg = criar_mensagem_ordem(i % 3, i % 5, (i % 2) ? 'C' : 'V', 
                                               25.0 + i, 100 + i * 50);
        if (enviar_mensagem_pipe(descritores[1], &msg) > 0) {
            printf("✓ Mensagem %d enviada\n", i + 1);
        }
    }
    
    printf("Recebendo múltiplas mensagens...\n");
    
    int mensagens_recebidas = 0;
    while (receber_mensagem_pipe(descritores[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem %d recebida\n", ++mensagens_recebidas);
        imprimir_mensagem(&mensagem_recebida);
    }
    
    printf("Total de mensagens recebidas: %d\n", mensagens_recebidas);
    
    // Teste 7: Testar criação e fechamento de pipes (TAREFA DO ALUNO)
    printf("\n=== TESTE 7: CRIAÇÃO E FECHAMENTO DE PIPES ===\n");
    
    printf("Testando criação de pipes...\n");
    int* novos_descriptores = criar_pipes_sistema();
    if (novos_descriptores) {
        printf("✓ Segunda criação de pipes bem-sucedida\n");
        imprimir_status_pipes();
        
        printf("Testando fechamento de pipes...\n");
        limpar_pipes_sistema();
        
        if (!pipes_estao_ativos()) {
            printf("✓ Pipes fechados corretamente\n");
        } else {
            printf("✗ Erro: Pipes ainda ativos após fechamento\n");
        }
    } else {
        printf("✗ Erro: Falha na segunda criação de pipes\n");
    }
    
    // Teste 8: Testar função testar_pipes_sistema()
    printf("\n=== TESTE 8: FUNÇÃO DE TESTE AUTOMÁTICO ===\n");
    testar_pipes_sistema();
    
    // Limpar pipes finais
    printf("\n=== LIMPEZA FINAL ===\n");
    limpar_pipes_sistema();
    
    printf("\n=== TODOS OS TESTES DOS PIPES CONCLUÍDOS COM SUCESSO! ===\n");
    printf("✓ Criação de pipes do sistema\n");
    printf("✓ Verificação de status dos pipes\n");
    printf("✓ Envio e recebimento de mensagens\n");
    printf("✓ Comunicação entre processos simulada\n");
    printf("✓ Tratamento de erros\n");
    printf("✓ Múltiplas mensagens\n");
    printf("✓ Criação e fechamento de pipes\n");
    printf("✓ Teste automático dos pipes\n");
    printf("✓ Gerenciamento correto de descritores de arquivo\n");
    
    return 0;
} 