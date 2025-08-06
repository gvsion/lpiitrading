#include "trading_system.h"
#include <math.h>
#include <unistd.h>

// Estrutura para dados do mercado
typedef struct {
    time_t horario_abertura;
    time_t horario_fechamento;
    int mercado_aberto;
    int volume_total;
    double valor_total_negociado;
    int num_operacoes;
} DadosMercado;

static DadosMercado dados_mercado_global;

// Preços iniciais realistas das ações brasileiras (baseados em dados reais)
static const double PRECOS_INICIAIS[] = {
    25.50,  // PETR4 - Petrobras
    68.30,  // VALE3 - Vale
    32.15,  // ITUB4 - Itaú
    14.20,  // ABEV3 - Ambev
    45.80,  // BBAS3 - Banco do Brasil
    15.80,  // BBDC4 - Bradesco
    45.90,  // WEGE3 - WEG
    55.40,  // RENT3 - Localiza
    18.75,  // LREN3 - Lojas Renner
    3.25,   // MGLU3 - Magazine Luiza
    22.10,  // JBSS3 - JBS
    35.60,  // SUZB3 - Suzano
    28.45   // GGBR4 - Gerdau
};

// Nomes das ações
static const char* NOMES_ACOES[] = {
    "PETR4", "VALE3", "ITUB4", "ABEV3", "BBAS3", "BBDC4", 
    "WEGE3", "RENT3", "LREN3", "MGLU3", "JBSS3", "SUZB3", "GGBR4"
};

// Setores das ações
static const char* SETORES_ACOES[] = {
    "Petróleo", "Mineração", "Bancos", "Bebidas", "Bancos", "Bancos",
    "Industrial", "Aluguel", "Varejo", "Varejo", "Alimentos", "Papel", "Siderurgia"
};

// Volatilidades das ações (baseadas em dados reais)
static const double VOLATILIDADES[] = {
    0.025, 0.035, 0.020, 0.030, 0.022, 0.028,
    0.018, 0.032, 0.040, 0.050, 0.038, 0.042, 0.045
};

// Função para inicializar dados do mercado
void inicializar_dados_mercado() {
    // Configurar horário de abertura (9:00) e fechamento (17:00)
    time_t agora = time(NULL);
    struct tm* tm_info = localtime(&agora);
    
    // Definir horário de abertura (9:00)
    tm_info->tm_hour = 9;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    dados_mercado_global.horario_abertura = mktime(tm_info);
    
    // Definir horário de fechamento (17:00)
    tm_info->tm_hour = 17;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    dados_mercado_global.horario_fechamento = mktime(tm_info);
    
    // Inicializar estatísticas
    dados_mercado_global.mercado_aberto = 1;
    dados_mercado_global.volume_total = 0;
    dados_mercado_global.valor_total_negociado = 0.0;
    dados_mercado_global.num_operacoes = 0;
    
    printf("=== MERCADO INICIALIZADO ===\n");
    printf("Horário de abertura: %s", ctime(&dados_mercado_global.horario_abertura));
    printf("Horário de fechamento: %s", ctime(&dados_mercado_global.horario_fechamento));
    printf("Status: %s\n", dados_mercado_global.mercado_aberto ? "ABERTO" : "FECHADO");
    printf("===========================\n\n");
}

// Função para inicializar ações com preços realistas
void inicializar_acoes_mercado(TradingSystem* sistema) {
    if (!sistema) return;
    
    // Definir número de ações (13 ações)
    sistema->num_acoes = 13;
    
    printf("=== INICIALIZANDO AÇÕES DO MERCADO ===\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        
        // Configurar nome
        strncpy(acao->nome, NOMES_ACOES[i], MAX_NOME - 1);
        acao->nome[MAX_NOME - 1] = '\0';
        
        // Configurar setor
        strncpy(acao->setor, SETORES_ACOES[i], MAX_NOME - 1);
        acao->setor[MAX_NOME - 1] = '\0';
        
        // Configurar preços
        acao->preco_atual = PRECOS_INICIAIS[i];
        acao->preco_anterior = PRECOS_INICIAIS[i];
        acao->preco_maximo = PRECOS_INICIAIS[i];
        acao->preco_minimo = PRECOS_INICIAIS[i];
        
        // Configurar volatilidade
        acao->volatilidade = VOLATILIDADES[i];
        
        // Inicializar estatísticas
        acao->volume_diario = 0;
        acao->volume_total = 0;
        acao->num_operacoes = 0;
        acao->variacao_diaria = 0.0;
        acao->variacao_semanal = 0.0;
        acao->variacao_mensal = 0.0;
        
        // Inicializar histórico de preços
        for (int j = 0; j < 30; j++) {
            acao->historico_precos[j] = PRECOS_INICIAIS[i];
        }
        acao->indice_historico = 0;
        
        // Inicializar mutex
        pthread_mutex_init(&acao->mutex, NULL);
        
        printf("✓ %s (%s) - R$ %.2f\n", acao->nome, acao->setor, acao->preco_atual);
    }
    
    printf("=== %d AÇÕES INICIALIZADAS ===\n\n", sistema->num_acoes);
}

// Função para verificar se o mercado está aberto
int mercado_esta_aberto() {
    time_t agora = time(NULL);
    struct tm* tm_agora = localtime(&agora);
    time_t horario_atual = mktime(tm_agora);
    
    // Verificar se é dia útil (segunda a sexta)
    if (tm_agora->tm_wday == 0 || tm_agora->tm_wday == 6) {
        return 0; // Fim de semana
    }
    
    // Verificar horário
    if (horario_atual >= dados_mercado_global.horario_abertura && 
        horario_atual <= dados_mercado_global.horario_fechamento) {
        return 1; // Mercado aberto
    }
    
    return 0; // Mercado fechado
}

// Função para obter horário de abertura formatado
char* obter_horario_abertura() {
    static char horario_str[20];
    struct tm* tm_info = localtime(&dados_mercado_global.horario_abertura);
    strftime(horario_str, sizeof(horario_str), "%H:%M", tm_info);
    return horario_str;
}

// Função para obter horário de fechamento formatado
char* obter_horario_fechamento() {
    static char horario_str[20];
    struct tm* tm_info = localtime(&dados_mercado_global.horario_fechamento);
    strftime(horario_str, sizeof(horario_str), "%H:%M", tm_info);
    return horario_str;
}

// Função para imprimir estado do mercado (TAREFA DO ALUNO)
void imprimir_estado_mercado(TradingSystem* sistema) {
    if (!sistema) return;
    
    printf("\n=== ESTADO DO MERCADO ===\n");
    
    // Informações gerais do mercado
    printf("📊 INFORMAÇÕES GERAIS:\n");
    printf("  Horário de abertura: %s\n", obter_horario_abertura());
    printf("  Horário de fechamento: %s\n", obter_horario_fechamento());
    printf("  Status: %s\n", mercado_esta_aberto() ? "🟢 ABERTO" : "🔴 FECHADO");
    printf("  Volume total: %d ações\n", dados_mercado_global.volume_total);
    printf("  Valor negociado: R$ %.2f\n", dados_mercado_global.valor_total_negociado);
    printf("  Operações: %d\n", dados_mercado_global.num_operacoes);
    
    // Estado das ações
    printf("\n📈 ESTADO DAS AÇÕES:\n");
    printf("%-8s %-12s %-10s %-8s %-8s %-8s %-8s\n", 
           "CÓDIGO", "SETOR", "PREÇO", "VAR%", "VOLUME", "MÁX", "MÍN");
    printf("-------- ------------ ---------- -------- -------- -------- --------\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        double variacao = ((acao->preco_atual - acao->preco_anterior) / acao->preco_anterior) * 100;
        char variacao_str[10];
        
        if (variacao > 0) {
            sprintf(variacao_str, "+%.2f%%", variacao);
        } else if (variacao < 0) {
            sprintf(variacao_str, "%.2f%%", variacao);
        } else {
            sprintf(variacao_str, "0.00%%");
        }
        
        printf("%-8s %-12s R$ %-6.2f %-8s %-8d R$ %-6.2f R$ %-6.2f\n",
               acao->nome, acao->setor, acao->preco_atual, variacao_str,
               acao->volume_diario, acao->preco_maximo, acao->preco_minimo);
    }
    
    // Top 5 ações por volume
    printf("\n🏆 TOP 5 POR VOLUME:\n");
    int indices[13];
    for (int i = 0; i < sistema->num_acoes; i++) {
        indices[i] = i;
    }
    
    // Ordenar por volume (bubble sort simples)
    for (int i = 0; i < sistema->num_acoes - 1; i++) {
        for (int j = 0; j < sistema->num_acoes - i - 1; j++) {
            if (sistema->acoes[indices[j]].volume_diario < sistema->acoes[indices[j + 1]].volume_diario) {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
    
    for (int i = 0; i < 5 && i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[indices[i]];
        printf("  %d. %s - %d ações - R$ %.2f\n", 
               i + 1, acao->nome, acao->volume_diario, acao->preco_atual);
    }
    
    // Top 5 ações por variação
    printf("\n📊 TOP 5 POR VARIAÇÃO:\n");
    for (int i = 0; i < sistema->num_acoes - 1; i++) {
        for (int j = 0; j < sistema->num_acoes - i - 1; j++) {
            double var1 = ((sistema->acoes[indices[j]].preco_atual - sistema->acoes[indices[j]].preco_anterior) / 
                          sistema->acoes[indices[j]].preco_anterior) * 100;
            double var2 = ((sistema->acoes[indices[j + 1]].preco_atual - sistema->acoes[indices[j + 1]].preco_anterior) / 
                          sistema->acoes[indices[j + 1]].preco_anterior) * 100;
            
            if (var1 < var2) {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
    
    for (int i = 0; i < 5 && i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[indices[i]];
        double variacao = ((acao->preco_atual - acao->preco_anterior) / acao->preco_anterior) * 100;
        printf("  %d. %s - %+.2f%% - R$ %.2f\n", 
               i + 1, acao->nome, variacao, acao->preco_atual);
    }
    
    // Estatísticas por setor
    printf("\n🏭 ESTATÍSTICAS POR SETOR:\n");
    char setores_unicos[10][20];
    int num_setores = 0;
    
    // Coletar setores únicos
    for (int i = 0; i < sistema->num_acoes; i++) {
        int encontrado = 0;
        for (int j = 0; j < num_setores; j++) {
            if (strcmp(sistema->acoes[i].setor, setores_unicos[j]) == 0) {
                encontrado = 1;
                break;
            }
        }
        if (!encontrado) {
            strcpy(setores_unicos[num_setores], sistema->acoes[i].setor);
            num_setores++;
        }
    }
    
    // Calcular estatísticas por setor
    for (int s = 0; s < num_setores; s++) {
        int num_acoes_setor = 0;
        double preco_medio = 0.0;
        int volume_total = 0;
        
        for (int i = 0; i < sistema->num_acoes; i++) {
            if (strcmp(sistema->acoes[i].setor, setores_unicos[s]) == 0) {
                num_acoes_setor++;
                preco_medio += sistema->acoes[i].preco_atual;
                volume_total += sistema->acoes[i].volume_diario;
            }
        }
        
        if (num_acoes_setor > 0) {
            preco_medio /= num_acoes_setor;
            printf("  %s: %d ações, preço médio R$ %.2f, volume %d\n",
                   setores_unicos[s], num_acoes_setor, preco_medio, volume_total);
        }
    }
    
    printf("===========================\n\n");
}

// Função para atualizar estatísticas do mercado
void atualizar_estatisticas_mercado(TradingSystem* sistema, Ordem* ordem) {
    if (!sistema || !ordem) return;
    
    dados_mercado_global.volume_total += ordem->quantidade;
    dados_mercado_global.valor_total_negociado += ordem->preco * ordem->quantidade;
    dados_mercado_global.num_operacoes++;
    
    // Atualizar estatísticas da ação
    Acao* acao = &sistema->acoes[ordem->acao_id];
    acao->volume_diario += ordem->quantidade;
    acao->volume_total += ordem->quantidade;
    acao->num_operacoes++;
    
    // Atualizar preços máximos e mínimos
    if (ordem->preco > acao->preco_maximo) {
        acao->preco_maximo = ordem->preco;
    }
    if (ordem->preco < acao->preco_minimo) {
        acao->preco_minimo = ordem->preco;
    }
}

// Função para resetar estatísticas diárias
void resetar_estatisticas_diarias(TradingSystem* sistema) {
    if (!sistema) return;
    
    printf("🔄 Resetando estatísticas diárias...\n");
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        acao->preco_anterior = acao->preco_atual;
        acao->volume_diario = 0;
        acao->variacao_diaria = 0.0;
        acao->preco_maximo = acao->preco_atual;
        acao->preco_minimo = acao->preco_atual;
    }
    
    dados_mercado_global.volume_total = 0;
    dados_mercado_global.valor_total_negociado = 0.0;
    dados_mercado_global.num_operacoes = 0;
    
    printf("✅ Estatísticas diárias resetadas\n");
}

// Função para simular abertura do mercado
void simular_abertura_mercado(TradingSystem* sistema) {
    printf("\n🔔 SIMULANDO ABERTURA DO MERCADO\n");
    printf("Horário: %s\n", obter_horario_abertura());
    
    // Resetar estatísticas
    resetar_estatisticas_diarias(sistema);
    
    // Simular pequenas variações nos preços de abertura
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        
        // Variação aleatória de ±2%
        double variacao = (rand() % 400 - 200) / 10000.0;
        acao->preco_atual = PRECOS_INICIAIS[i] * (1.0 + variacao);
        acao->preco_anterior = acao->preco_atual;
        acao->preco_maximo = acao->preco_atual;
        acao->preco_minimo = acao->preco_atual;
    }
    
    printf("✅ Mercado aberto com preços atualizados\n");
}

// Função para simular fechamento do mercado
void simular_fechamento_mercado(TradingSystem* sistema) {
    printf("\n🔔 SIMULANDO FECHAMENTO DO MERCADO\n");
    printf("Horário: %s\n", obter_horario_fechamento());
    
    // Calcular variações finais
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        acao->variacao_diaria = ((acao->preco_atual - acao->preco_anterior) / acao->preco_anterior) * 100;
    }
    
    printf("📊 RESUMO DO DIA:\n");
    printf("Volume total: %d ações\n", dados_mercado_global.volume_total);
    printf("Valor negociado: R$ %.2f\n", dados_mercado_global.valor_total_negociado);
    printf("Operações: %d\n", dados_mercado_global.num_operacoes);
    
    printf("✅ Mercado fechado\n");
}

// Função para obter dados do mercado
DadosMercado* obter_dados_mercado() {
    return &dados_mercado_global;
} 