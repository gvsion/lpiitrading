#include "trading_system.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// Variável global para gerenciar pipes do sistema
static SistemaPipes sistema_pipes;

// Função para criar pipes do sistema (TAREFA DO ALUNO)
int* criar_pipes_sistema() {
    printf("=== CRIANDO PIPES DO SISTEMA ===\n");
    
    // Inicializar estrutura
    memset(&sistema_pipes, 0, sizeof(SistemaPipes));
    
    // Array para retornar descritores
    static int descritores[10]; // 5 pipes * 2 descritores cada
    int indice = 0;
    
    // 1. Pipe Traders -> Executor
    printf("Criando pipe Traders -> Executor...\n");
    if (pipe(sistema_pipes.traders_to_executor) == -1) {
        perror("ERRO: Falha ao criar pipe Traders -> Executor");
        return NULL;
    }
    descritores[indice++] = sistema_pipes.traders_to_executor[0];
    descritores[indice++] = sistema_pipes.traders_to_executor[1];
    sistema_pipes.num_pipes_criados++;
    printf("✓ Pipe Traders -> Executor criado (RD: %d, WR: %d)\n", 
           sistema_pipes.traders_to_executor[0], sistema_pipes.traders_to_executor[1]);
    
    // 2. Pipe Executor -> Price Updater
    printf("Criando pipe Executor -> Price Updater...\n");
    if (pipe(sistema_pipes.executor_to_price_updater) == -1) {
        perror("ERRO: Falha ao criar pipe Executor -> Price Updater");
        limpar_pipes_sistema();
        return NULL;
    }
    descritores[indice++] = sistema_pipes.executor_to_price_updater[0];
    descritores[indice++] = sistema_pipes.executor_to_price_updater[1];
    sistema_pipes.num_pipes_criados++;
    printf("✓ Pipe Executor -> Price Updater criado (RD: %d, WR: %d)\n", 
           sistema_pipes.executor_to_price_updater[0], sistema_pipes.executor_to_price_updater[1]);
    
    // 3. Pipe Price Updater -> Arbitrage Monitor
    printf("Criando pipe Price Updater -> Arbitrage Monitor...\n");
    if (pipe(sistema_pipes.price_updater_to_arbitrage) == -1) {
        perror("ERRO: Falha ao criar pipe Price Updater -> Arbitrage Monitor");
        limpar_pipes_sistema();
        return NULL;
    }
    descritores[indice++] = sistema_pipes.price_updater_to_arbitrage[0];
    descritores[indice++] = sistema_pipes.price_updater_to_arbitrage[1];
    sistema_pipes.num_pipes_criados++;
    printf("✓ Pipe Price Updater -> Arbitrage Monitor criado (RD: %d, WR: %d)\n", 
           sistema_pipes.price_updater_to_arbitrage[0], sistema_pipes.price_updater_to_arbitrage[1]);
    
    // 4. Pipe Arbitrage Monitor -> Traders (feedback)
    printf("Criando pipe Arbitrage Monitor -> Traders...\n");
    if (pipe(sistema_pipes.arbitrage_to_traders) == -1) {
        perror("ERRO: Falha ao criar pipe Arbitrage Monitor -> Traders");
        limpar_pipes_sistema();
        return NULL;
    }
    descritores[indice++] = sistema_pipes.arbitrage_to_traders[0];
    descritores[indice++] = sistema_pipes.arbitrage_to_traders[1];
    sistema_pipes.num_pipes_criados++;
    printf("✓ Pipe Arbitrage Monitor -> Traders criado (RD: %d, WR: %d)\n", 
           sistema_pipes.arbitrage_to_traders[0], sistema_pipes.arbitrage_to_traders[1]);
    
    // 5. Pipe de Controle
    printf("Criando pipe de controle...\n");
    if (pipe(sistema_pipes.control_pipe) == -1) {
        perror("ERRO: Falha ao criar pipe de controle");
        limpar_pipes_sistema();
        return NULL;
    }
    descritores[indice++] = sistema_pipes.control_pipe[0];
    descritores[indice++] = sistema_pipes.control_pipe[1];
    sistema_pipes.num_pipes_criados++;
    printf("✓ Pipe de controle criado (RD: %d, WR: %d)\n", 
           sistema_pipes.control_pipe[0], sistema_pipes.control_pipe[1]);
    
    // Configurar pipes como não-bloqueantes
    for (int i = 0; i < 10; i++) {
        int flags = fcntl(descritores[i], F_GETFL, 0);
        fcntl(descritores[i], F_SETFL, flags | O_NONBLOCK);
    }
    
    sistema_pipes.pipes_ativos = 1;
    printf("=== %d PIPES CRIADOS COM SUCESSO ===\n\n", sistema_pipes.num_pipes_criados);
    
    return descritores;
}

// Função para limpar pipes do sistema
void limpar_pipes_sistema() {
    printf("=== LIMPANDO PIPES DO SISTEMA ===\n");
    
    // Fechar todos os descritores
    if (sistema_pipes.traders_to_executor[0] > 0) {
        close(sistema_pipes.traders_to_executor[0]);
        printf("✓ Fechado pipe Traders->Executor (RD)\n");
    }
    if (sistema_pipes.traders_to_executor[1] > 0) {
        close(sistema_pipes.traders_to_executor[1]);
        printf("✓ Fechado pipe Traders->Executor (WR)\n");
    }
    
    if (sistema_pipes.executor_to_price_updater[0] > 0) {
        close(sistema_pipes.executor_to_price_updater[0]);
        printf("✓ Fechado pipe Executor->PriceUpdater (RD)\n");
    }
    if (sistema_pipes.executor_to_price_updater[1] > 0) {
        close(sistema_pipes.executor_to_price_updater[1]);
        printf("✓ Fechado pipe Executor->PriceUpdater (WR)\n");
    }
    
    if (sistema_pipes.price_updater_to_arbitrage[0] > 0) {
        close(sistema_pipes.price_updater_to_arbitrage[0]);
        printf("✓ Fechado pipe PriceUpdater->Arbitrage (RD)\n");
    }
    if (sistema_pipes.price_updater_to_arbitrage[1] > 0) {
        close(sistema_pipes.price_updater_to_arbitrage[1]);
        printf("✓ Fechado pipe PriceUpdater->Arbitrage (WR)\n");
    }
    
    if (sistema_pipes.arbitrage_to_traders[0] > 0) {
        close(sistema_pipes.arbitrage_to_traders[0]);
        printf("✓ Fechado pipe Arbitrage->Traders (RD)\n");
    }
    if (sistema_pipes.arbitrage_to_traders[1] > 0) {
        close(sistema_pipes.arbitrage_to_traders[1]);
        printf("✓ Fechado pipe Arbitrage->Traders (WR)\n");
    }
    
    if (sistema_pipes.control_pipe[0] > 0) {
        close(sistema_pipes.control_pipe[0]);
        printf("✓ Fechado pipe de controle (RD)\n");
    }
    if (sistema_pipes.control_pipe[1] > 0) {
        close(sistema_pipes.control_pipe[1]);
        printf("✓ Fechado pipe de controle (WR)\n");
    }
    
    sistema_pipes.pipes_ativos = 0;
    printf("=== TODOS OS PIPES FECHADOS ===\n\n");
}

// Função para enviar mensagem via pipe
int enviar_mensagem_pipe(int pipe_write, void* mensagem) {
    if (!mensagem || pipe_write <= 0) {
        printf("ERRO: Parâmetros inválidos para envio de mensagem\n");
        return -1;
    }
    
    MensagemPipe* msg = (MensagemPipe*)mensagem;
    msg->timestamp = time(NULL);
    ssize_t bytes_escritos = write(pipe_write, msg, sizeof(MensagemPipe));
    
    if (bytes_escritos == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Pipe cheio, não é erro crítico
            return 0;
        } else {
            perror("ERRO: Falha ao escrever no pipe");
            return -1;
        }
    } else if (bytes_escritos != sizeof(MensagemPipe)) {
        printf("ERRO: Mensagem incompleta enviada (%zd bytes)\n", bytes_escritos);
        return -1;
    }
    
    return 1; // Sucesso
}

// Função para receber mensagem via pipe
int receber_mensagem_pipe(int pipe_read, void* mensagem) {
    if (!mensagem || pipe_read <= 0) {
        printf("ERRO: Parâmetros inválidos para recebimento de mensagem\n");
        return -1;
    }
    
    MensagemPipe* msg = (MensagemPipe*)mensagem;
    ssize_t bytes_lidos = read(pipe_read, msg, sizeof(MensagemPipe));
    
    if (bytes_lidos == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Nenhuma mensagem disponível
            return 0;
        } else {
            perror("ERRO: Falha ao ler do pipe");
            return -1;
        }
    } else if (bytes_lidos == 0) {
        // Pipe fechado
        return -1;
    } else if (bytes_lidos != sizeof(MensagemPipe)) {
        printf("ERRO: Mensagem incompleta recebida (%zd bytes)\n", bytes_lidos);
        return -1;
    }
    
    return 1; // Sucesso
}

// Função para obter descritores de pipe
SistemaPipes* obter_pipes_sistema() {
    return &sistema_pipes;
}

// Função para verificar se pipes estão ativos
int pipes_estao_ativos() {
    return sistema_pipes.pipes_ativos;
}

// Função para imprimir status dos pipes
void imprimir_status_pipes() {
    printf("=== STATUS DOS PIPES ===\n");
    printf("Pipes criados: %d\n", sistema_pipes.num_pipes_criados);
    printf("Pipes ativos: %s\n", sistema_pipes.pipes_ativos ? "SIM" : "NÃO");
    
    if (sistema_pipes.pipes_ativos) {
        printf("Descritores dos pipes:\n");
        printf("  Traders->Executor: RD=%d, WR=%d\n", 
               sistema_pipes.traders_to_executor[0], sistema_pipes.traders_to_executor[1]);
        printf("  Executor->PriceUpdater: RD=%d, WR=%d\n", 
               sistema_pipes.executor_to_price_updater[0], sistema_pipes.executor_to_price_updater[1]);
        printf("  PriceUpdater->Arbitrage: RD=%d, WR=%d\n", 
               sistema_pipes.price_updater_to_arbitrage[0], sistema_pipes.price_updater_to_arbitrage[1]);
        printf("  Arbitrage->Traders: RD=%d, WR=%d\n", 
               sistema_pipes.arbitrage_to_traders[0], sistema_pipes.arbitrage_to_traders[1]);
        printf("  Controle: RD=%d, WR=%d\n", 
               sistema_pipes.control_pipe[0], sistema_pipes.control_pipe[1]);
    }
    printf("========================\n\n");
}

// Função para criar mensagem de ordem
MensagemPipe criar_mensagem_ordem(int trader_id, int acao_id, char tipo, double preco, int quantidade) {
    MensagemPipe mensagem;
    memset(&mensagem, 0, sizeof(MensagemPipe));
    
    mensagem.tipo_mensagem = 1; // Ordem
    mensagem.origem_id = trader_id;
    mensagem.destino_id = 0; // Executor
    mensagem.valor = preco;
    mensagem.dados_ordem = (acao_id << 16) | (tipo << 8) | quantidade;
    mensagem.timestamp = time(NULL);
    
    snprintf(mensagem.dados_extras, sizeof(mensagem.dados_extras), 
             "Ordem: %c %d ações de %d a R$ %.2f", tipo, quantidade, acao_id, preco);
    
    return mensagem;
}

// Função para criar mensagem de atualização de preço
MensagemPipe criar_mensagem_atualizacao_preco(int acao_id, double preco_anterior, double preco_novo) {
    MensagemPipe mensagem;
    memset(&mensagem, 0, sizeof(MensagemPipe));
    
    mensagem.tipo_mensagem = 2; // Atualização de preço
    mensagem.origem_id = 1; // Executor
    mensagem.destino_id = 2; // Price Updater
    mensagem.valor = preco_novo;
    mensagem.dados_ordem = acao_id;
    mensagem.timestamp = time(NULL);
    
    snprintf(mensagem.dados_extras, sizeof(mensagem.dados_extras), 
             "Preço %d: R$ %.2f -> R$ %.2f (variação: %.2f%%)", 
             acao_id, preco_anterior, preco_novo, 
             ((preco_novo - preco_anterior) / preco_anterior) * 100);
    
    return mensagem;
}

// Função para criar mensagem de arbitragem
MensagemPipe criar_mensagem_arbitragem(int acao1_id, int acao2_id, double diferenca, double percentual) {
    MensagemPipe mensagem;
    memset(&mensagem, 0, sizeof(MensagemPipe));
    
    mensagem.tipo_mensagem = 3; // Arbitragem
    mensagem.origem_id = 2; // Price Updater
    mensagem.destino_id = 3; // Arbitrage Monitor
    mensagem.valor = diferenca;
    mensagem.dados_ordem = (acao1_id << 16) | acao2_id;
    mensagem.timestamp = time(NULL);
    
    snprintf(mensagem.dados_extras, sizeof(mensagem.dados_extras), 
             "Arbitragem: %d vs %d, dif: R$ %.2f (%.2f%%)", 
             acao1_id, acao2_id, diferenca, percentual);
    
    return mensagem;
}

// Função para criar mensagem de controle
MensagemPipe criar_mensagem_controle(int comando, int origem_id, int destino_id) {
    MensagemPipe mensagem;
    memset(&mensagem, 0, sizeof(MensagemPipe));
    
    mensagem.tipo_mensagem = 4; // Controle
    mensagem.origem_id = origem_id;
    mensagem.destino_id = destino_id;
    mensagem.dados_ordem = comando;
    mensagem.timestamp = time(NULL);
    
    snprintf(mensagem.dados_extras, sizeof(mensagem.dados_extras), 
             "Comando: %d de %d para %d", comando, origem_id, destino_id);
    
    return mensagem;
}

// Função para imprimir mensagem
void imprimir_mensagem(MensagemPipe* mensagem) {
    if (!mensagem) return;
    
    char* tipo_str;
    switch (mensagem->tipo_mensagem) {
        case 1: tipo_str = "ORDEM"; break;
        case 2: tipo_str = "ATUALIZAÇÃO"; break;
        case 3: tipo_str = "ARBITRAGEM"; break;
        case 4: tipo_str = "CONTROLE"; break;
        default: tipo_str = "DESCONHECIDO"; break;
    }
    
    char* timestamp = ctime(&mensagem->timestamp);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remover \n
    
    printf("[%s] %s: %d -> %d | Valor: %.2f | %s\n", 
           timestamp, tipo_str, mensagem->origem_id, mensagem->destino_id, 
           mensagem->valor, mensagem->dados_extras);
}

// Função para testar pipes
void testar_pipes_sistema() {
    printf("=== TESTE DOS PIPES DO SISTEMA ===\n");
    
    // 1. Criar pipes
    int* descritores = criar_pipes_sistema();
    if (!descritores) {
        printf("ERRO: Falha ao criar pipes\n");
        return;
    }
    
    imprimir_status_pipes();
    
    // 2. Testar envio de mensagens
    printf("Testando envio de mensagens...\n");
    
    // Mensagem de ordem
    MensagemPipe ordem = criar_mensagem_ordem(0, 1, 'C', 25.50, 100);
    if (enviar_mensagem_pipe(sistema_pipes.traders_to_executor[1], &ordem) > 0) {
        printf("✓ Mensagem de ordem enviada\n");
        imprimir_mensagem(&ordem);
    }
    
    // Mensagem de atualização
    MensagemPipe atualizacao = criar_mensagem_atualizacao_preco(1, 25.50, 26.00);
    if (enviar_mensagem_pipe(sistema_pipes.executor_to_price_updater[1], &atualizacao) > 0) {
        printf("✓ Mensagem de atualização enviada\n");
        imprimir_mensagem(&atualizacao);
    }
    
    // Mensagem de arbitragem
    MensagemPipe arbitragem = criar_mensagem_arbitragem(1, 2, 5.50, 20.0);
    if (enviar_mensagem_pipe(sistema_pipes.price_updater_to_arbitrage[1], &arbitragem) > 0) {
        printf("✓ Mensagem de arbitragem enviada\n");
        imprimir_mensagem(&arbitragem);
    }
    
    // Mensagem de controle
    MensagemPipe controle = criar_mensagem_controle(1, 0, 3);
    if (enviar_mensagem_pipe(sistema_pipes.control_pipe[1], &controle) > 0) {
        printf("✓ Mensagem de controle enviada\n");
        imprimir_mensagem(&controle);
    }
    
    // 3. Testar recebimento de mensagens
    printf("\nTestando recebimento de mensagens...\n");
    
    MensagemPipe mensagem_recebida;
    
    // Receber ordem
    if (receber_mensagem_pipe(sistema_pipes.traders_to_executor[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de ordem recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    // Receber atualização
    if (receber_mensagem_pipe(sistema_pipes.executor_to_price_updater[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de atualização recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    // Receber arbitragem
    if (receber_mensagem_pipe(sistema_pipes.price_updater_to_arbitrage[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de arbitragem recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    // Receber controle
    if (receber_mensagem_pipe(sistema_pipes.control_pipe[0], &mensagem_recebida) > 0) {
        printf("✓ Mensagem de controle recebida\n");
        imprimir_mensagem(&mensagem_recebida);
    }
    
    // 4. Limpar pipes
    printf("\nLimpando pipes...\n");
    limpar_pipes_sistema();
    
    printf("=== TESTE DOS PIPES CONCLUÍDO ===\n\n");
} 