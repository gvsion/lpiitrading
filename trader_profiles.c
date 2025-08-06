#define _POSIX_C_SOURCE 200809L
#include "trading_system.h"
#include <poll.h>
#include <sys/time.h>

// Array global de perfis de trader
static PerfilTrader perfis_trader[3];

// Função para inicializar perfis de trader
void inicializar_perfis_trader() {
    printf("=== INICIALIZANDO PERFIS DE TRADER ===\n");
    
    // Perfil Conservador
    perfis_trader[PERFIL_CONSERVADOR].perfil_id = PERFIL_CONSERVADOR;
    strcpy(perfis_trader[PERFIL_CONSERVADOR].nome, "Conservador");
    perfis_trader[PERFIL_CONSERVADOR].intervalo_min_ordens = 3;
    perfis_trader[PERFIL_CONSERVADOR].intervalo_max_ordens = 8;
    perfis_trader[PERFIL_CONSERVADOR].max_ordens_por_sessao = 20;
    perfis_trader[PERFIL_CONSERVADOR].tempo_limite_sessao = 300;
    perfis_trader[PERFIL_CONSERVADOR].agressividade = 0.3;
    perfis_trader[PERFIL_CONSERVADOR].volume_medio = 100;
    perfis_trader[PERFIL_CONSERVADOR].acoes_preferidas[0] = 0; // PETR4
    perfis_trader[PERFIL_CONSERVADOR].acoes_preferidas[1] = 1; // VALE3
    perfis_trader[PERFIL_CONSERVADOR].num_acoes_preferidas = 2;
    
    // Perfil Agressivo
    perfis_trader[PERFIL_AGRESSIVO].perfil_id = PERFIL_AGRESSIVO;
    strcpy(perfis_trader[PERFIL_AGRESSIVO].nome, "Agressivo");
    perfis_trader[PERFIL_AGRESSIVO].intervalo_min_ordens = 1;
    perfis_trader[PERFIL_AGRESSIVO].intervalo_max_ordens = 4;
    perfis_trader[PERFIL_AGRESSIVO].max_ordens_por_sessao = 50;
    perfis_trader[PERFIL_AGRESSIVO].tempo_limite_sessao = 300;
    perfis_trader[PERFIL_AGRESSIVO].agressividade = 0.8;
    perfis_trader[PERFIL_AGRESSIVO].volume_medio = 500;
    perfis_trader[PERFIL_AGRESSIVO].acoes_preferidas[0] = 2; // ITUB4
    perfis_trader[PERFIL_AGRESSIVO].acoes_preferidas[1] = 3; // ABEV3
    perfis_trader[PERFIL_AGRESSIVO].acoes_preferidas[2] = 4; // BBAS3
    perfis_trader[PERFIL_AGRESSIVO].num_acoes_preferidas = 3;
    
    // Perfil Day Trader
    perfis_trader[PERFIL_DAY_TRADER].perfil_id = PERFIL_DAY_TRADER;
    strcpy(perfis_trader[PERFIL_DAY_TRADER].nome, "Day Trader");
    perfis_trader[PERFIL_DAY_TRADER].intervalo_min_ordens = 1;
    perfis_trader[PERFIL_DAY_TRADER].intervalo_max_ordens = 3;
    perfis_trader[PERFIL_DAY_TRADER].max_ordens_por_sessao = 100;
    perfis_trader[PERFIL_DAY_TRADER].tempo_limite_sessao = 300;
    perfis_trader[PERFIL_DAY_TRADER].agressividade = 0.9;
    perfis_trader[PERFIL_DAY_TRADER].volume_medio = 200;
    perfis_trader[PERFIL_DAY_TRADER].acoes_preferidas[0] = 5; // BBDC4
    perfis_trader[PERFIL_DAY_TRADER].acoes_preferidas[1] = 6; // WEGE3
    perfis_trader[PERFIL_DAY_TRADER].acoes_preferidas[2] = 7; // RENT3
    perfis_trader[PERFIL_DAY_TRADER].acoes_preferidas[3] = 8; // LREN3
    perfis_trader[PERFIL_DAY_TRADER].num_acoes_preferidas = 4;
    
    printf("✓ Perfis de trader inicializados:\n");
    for (int i = 0; i < 3; i++) {
        printf("  - %s (ID: %d)\n", perfis_trader[i].nome, perfis_trader[i].perfil_id);
    }
    printf("\n");
}

// Função para obter perfil de trader
PerfilTrader* obter_perfil_trader(int perfil_id) {
    if (perfil_id >= 0 && perfil_id < 3) {
        return &perfis_trader[perfil_id];
    }
    return NULL;
}

// Função para aplicar perfil a um trader
void aplicar_perfil_trader(TradingSystem* sistema, int trader_id, int perfil_id) {
    if (trader_id < 0 || trader_id >= MAX_TRADERS) {
        printf("ERRO: Trader ID inválido: %d\n", trader_id);
        return;
    }
    
    PerfilTrader* perfil = obter_perfil_trader(perfil_id);
    if (!perfil) {
        printf("ERRO: Perfil inválido: %d\n", perfil_id);
        return;
    }
    
    Trader* trader = &sistema->traders[trader_id];
    printf("Aplicando perfil '%s' ao trader %d (%s)\n", 
           perfil->nome, trader_id, trader->nome);
}

// Função para gerar intervalo aleatório
int gerar_intervalo_aleatorio(int min, int max) {
    return min + (rand() % (max - min + 1));
}

// Função para calcular probabilidade de compra baseada no preço
double calcular_probabilidade_compra(TradingSystem* sistema, int acao_id, PerfilTrader* perfil) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return 0.0;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    double variacao = acao->variacao;
    
    // Base: 30% de probabilidade
    double probabilidade = 0.3;
    
    // Se preço caiu, maior probabilidade de compra
    if (variacao < -0.02) { // Caiu mais de 2%
        probabilidade += 0.4;
    } else if (variacao < -0.01) { // Caiu mais de 1%
        probabilidade += 0.2;
    }
    
    // Ajustar pela agressividade do perfil
    probabilidade *= (1.0 + perfil->agressividade);
    
    // Limitar a 90%
    if (probabilidade > 0.9) probabilidade = 0.9;
    
    return probabilidade;
}

// Função para calcular probabilidade de venda baseada no preço
double calcular_probabilidade_venda(TradingSystem* sistema, int acao_id, PerfilTrader* perfil) {
    if (acao_id < 0 || acao_id >= sistema->num_acoes) {
        return 0.0;
    }
    
    Acao* acao = &sistema->acoes[acao_id];
    double variacao = acao->variacao;
    
    // Base: 20% de probabilidade
    double probabilidade = 0.2;
    
    // Se preço subiu, maior probabilidade de venda
    if (variacao > 0.02) { // Subiu mais de 2%
        probabilidade += 0.4;
    } else if (variacao > 0.01) { // Subiu mais de 1%
        probabilidade += 0.2;
    }
    
    // Ajustar pela agressividade do perfil
    probabilidade *= (1.0 + perfil->agressividade);
    
    // Limitar a 80%
    if (probabilidade > 0.8) probabilidade = 0.8;
    
    return probabilidade;
}

// Função para decidir ação do trader
int decidir_acao_trader(TradingSystem* sistema, int trader_id, PerfilTrader* perfil) {
    Trader* trader = &sistema->traders[trader_id];
    
    // Escolher ação aleatória das preferidas
    int acao_id = perfil->acoes_preferidas[rand() % perfil->num_acoes_preferidas];
    Acao* acao = &sistema->acoes[acao_id];
    
    double prob_compra = calcular_probabilidade_compra(sistema, acao_id, perfil);
    double prob_venda = calcular_probabilidade_venda(sistema, acao_id, perfil);
    
    double random = (double)rand() / RAND_MAX;
    
    // Decidir ação baseada nas probabilidades
    if (random < prob_compra && trader->saldo > acao->preco_atual * perfil->volume_medio) {
        // Comprar
        int quantidade = (int)(perfil->volume_medio * (0.8 + 0.4 * ((double)rand() / RAND_MAX)));
        criar_ordem(sistema, trader_id, acao_id, 'C', acao->preco_atual, quantidade);
        log_ordem_trader(trader_id, acao_id, 'C', acao->preco_atual, quantidade, "Probabilidade de compra");
        return 1; // Ordem criada
    } else if (random < (prob_compra + prob_venda) && trader->acoes_possuidas[acao_id] > 0) {
        // Vender
        int quantidade = trader->acoes_possuidas[acao_id] > perfil->volume_medio ? 
                        (int)perfil->volume_medio : trader->acoes_possuidas[acao_id];
        criar_ordem(sistema, trader_id, acao_id, 'V', acao->preco_atual, quantidade);
        log_ordem_trader(trader_id, acao_id, 'V', acao->preco_atual, quantidade, "Probabilidade de venda");
        return 1; // Ordem criada
    }
    
    return 0; // Nenhuma ordem criada
}

// Função para log detalhado de ordens
void log_ordem_trader(int trader_id, int acao_id, char tipo, double preco, int quantidade, const char* motivo) {
    (void)acao_id; // Evitar warning de parâmetro não utilizado
    time_t agora = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&agora));
    
    printf("[%s] TRADER %d: %s %d ações a R$ %.2f (%s)\n", 
           timestamp, trader_id, 
           tipo == 'C' ? "COMPRA" : "VENDA", 
           quantidade, preco, motivo);
}

// Função principal do processo trader melhorado
void processo_trader_melhorado(int trader_id, int perfil_id) {
    printf("=== PROCESSO TRADER %d INICIADO (PID: %d, Perfil: %d) ===\n", 
           trader_id, getpid(), perfil_id);
    
    // Anexar memória compartilhada
    TradingSystem* sistema = (TradingSystem*)shmat(shm_id, NULL, 0);
    if (sistema == (void*)-1) {
        perror("Erro ao anexar memória compartilhada no processo trader");
        exit(1);
    }
    
    // Obter perfil do trader
    PerfilTrader* perfil = obter_perfil_trader(perfil_id);
    if (!perfil) {
        printf("ERRO: Perfil inválido %d para trader %d\n", perfil_id, trader_id);
        shmdt(sistema);
        exit(1);
    }
    
    // Variáveis de controle
    int ordens_enviadas = 0;
    time_t inicio_sessao = time(NULL);
    time_t ultima_ordem = 0;
    
    printf("Trader %d iniciado com perfil '%s'\n", trader_id, perfil->nome);
    printf("Configurações: intervalo %d-%ds, max %d ordens, tempo limite %ds\n",
           perfil->intervalo_min_ordens, perfil->intervalo_max_ordens,
           perfil->max_ordens_por_sessao, perfil->tempo_limite_sessao);
    
    while (sistema->sistema_ativo) {
        time_t agora = time(NULL);
        
        // Verificar limites de tempo e ordens
        if (agora - inicio_sessao > perfil->tempo_limite_sessao) {
            printf("Trader %d: Tempo limite atingido (%ds)\n", trader_id, perfil->tempo_limite_sessao);
            break;
        }
        
        if (ordens_enviadas >= perfil->max_ordens_por_sessao) {
            printf("Trader %d: Limite de ordens atingido (%d)\n", trader_id, perfil->max_ordens_por_sessao);
            break;
        }
        
        // Verificar se é hora de enviar ordem
        int intervalo = gerar_intervalo_aleatorio(perfil->intervalo_min_ordens, perfil->intervalo_max_ordens);
        if (agora - ultima_ordem >= intervalo) {
            
            // Decidir ação do trader
            if (decidir_acao_trader(sistema, trader_id, perfil)) {
                ordens_enviadas++;
                ultima_ordem = agora;
                
                printf("Trader %d: Ordem criada (total: %d/%d)\n", 
                       trader_id, ordens_enviadas, perfil->max_ordens_por_sessao);
            }
        }
        
        // Pequena pausa para não sobrecarregar
        usleep(100000); // 0.1 segundo
    }
    
    // Estatísticas finais
    time_t duracao = time(NULL) - inicio_sessao;
    printf("=== TRADER %d FINALIZADO ===\n", trader_id);
    printf("Duração: %lds\n", duracao);
    printf("Ordens enviadas: %d/%d\n", ordens_enviadas, perfil->max_ordens_por_sessao);
    printf("Perfil: %s\n", perfil->nome);
    
    // Desanexar memória compartilhada
    shmdt(sistema);
    
    exit(0);
} 