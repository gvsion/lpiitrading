#include "trading_system.h"
#include <pthread.h>
#include <unistd.h>
#include <math.h>

// Estrutura para oportunidades de arbitragem
typedef struct {
    int acao_compra_id;
    int acao_venda_id;
    double preco_compra;
    double preco_venda;
    double spread_percentual;
    double lucro_potencial;
    int volume_disponivel;
    time_t timestamp;
    int executada;
    double lucro_realizado;
} OportunidadeArbitragem;

// Estrutura para estatísticas de arbitragem
typedef struct {
    int total_oportunidades_detectadas;
    int total_arbitragens_executadas;
    double lucro_total_potencial;
    double lucro_total_realizado;
    double maior_spread_detectado;
    double menor_spread_executado;
    int oportunidades_por_setor[10]; // Por setor
    pthread_mutex_t mutex;
} EstatisticasArbitragem;

// Estrutura para pares de ações relacionadas
typedef struct {
    int acao1_id;
    int acao2_id;
    char* setor;
    double spread_minimo;
    double correlacao_historica;
} ParAcoesRelacionadas;

// Dados globais para arbitragem
static OportunidadeArbitragem oportunidades[MAX_OPORTUNIDADES];
static EstatisticasArbitragem estatisticas_arbitragem;
static int num_oportunidades = 0;
static int arbitragem_ativa = 1;

// Pares de ações relacionadas pré-definidos
static ParAcoesRelacionadas pares_relacionadas[] = {
    {0, 1, "Petróleo", 0.02, 0.8},      // PETR4 vs VALE3 (diferentes setores mas correlacionados)
    {2, 4, "Bancos", 0.02, 0.9},        // ITUB4 vs BBAS3
    {2, 5, "Bancos", 0.02, 0.85},       // ITUB4 vs BBDC4
    {4, 5, "Bancos", 0.02, 0.9},        // BBAS3 vs BBDC4
    {3, 10, "Consumo", 0.02, 0.7},      // ABEV3 vs JBSS3
    {8, 9, "Varejo", 0.02, 0.75},       // LREN3 vs MGLU3
    {6, 7, "Industrial", 0.02, 0.6},    // WEGE3 vs RENT3
    {11, 12, "Industrial", 0.02, 0.65}, // SUZB3 vs GGBR4
    {-1, -1, NULL, 0.0, 0.0} // Terminador
};

// Função para inicializar estatísticas de arbitragem
void inicializar_estatisticas_arbitragem() {
    printf("=== INICIALIZANDO DETECTOR DE ARBITRAGEM ===\n");
    
    estatisticas_arbitragem.total_oportunidades_detectadas = 0;
    estatisticas_arbitragem.total_arbitragens_executadas = 0;
    estatisticas_arbitragem.lucro_total_potencial = 0.0;
    estatisticas_arbitragem.lucro_total_realizado = 0.0;
    estatisticas_arbitragem.maior_spread_detectado = 0.0;
    estatisticas_arbitragem.menor_spread_executado = 999.0;
    
    for (int i = 0; i < 10; i++) {
        estatisticas_arbitragem.oportunidades_por_setor[i] = 0;
    }
    
    pthread_mutex_init(&estatisticas_arbitragem.mutex, NULL);
    
    printf("✓ Estatísticas de arbitragem inicializadas\n");
    printf("✓ Critério de spread mínimo: 2%%\n");
    printf("✓ Monitoramento de %ld pares de ações relacionadas\n", 
           sizeof(pares_relacionadas) / sizeof(ParAcoesRelacionadas) - 1);
}

// Função para calcular spread entre duas ações
double calcular_spread(double preco1, double preco2) {
    if (preco1 <= 0 || preco2 <= 0) return 0.0;
    
    double spread = fabs(preco1 - preco2) / ((preco1 + preco2) / 2.0);
    return spread;
}

// Função para determinar qual ação comprar e qual vender
int determinar_acao_compra_venda(double preco1, double preco2, int acao1_id, int acao2_id, 
                                int* acao_compra, int* acao_venda, double* preco_compra, double* preco_venda) {
    if (preco1 < preco2) {
        *acao_compra = acao1_id;
        *acao_venda = acao2_id;
        *preco_compra = preco1;
        *preco_venda = preco2;
        return 1;
    } else {
        *acao_compra = acao2_id;
        *acao_venda = acao1_id;
        *preco_compra = preco2;
        *preco_venda = preco1;
        return 1;
    }
}

// Função para calcular lucro potencial
double calcular_lucro_potencial(double preco_compra, double preco_venda, int volume) {
    return (preco_venda - preco_compra) * volume;
}

// Função para detectar oportunidades de arbitragem
void detectar_oportunidades_arbitragem(TradingSystem* sistema) {
    for (int i = 0; pares_relacionadas[i].acao1_id != -1; i++) {
        ParAcoesRelacionadas* par = &pares_relacionadas[i];
        
        // Obter preços das ações
        double preco1 = sistema->acoes[par->acao1_id].preco_atual;
        double preco2 = sistema->acoes[par->acao2_id].preco_atual;
        
        // Calcular spread
        double spread = calcular_spread(preco1, preco2);
        
        // Verificar se spread é maior que o mínimo
        if (spread > par->spread_minimo) {
            int acao_compra, acao_venda;
            double preco_compra, preco_venda;
            
            if (determinar_acao_compra_venda(preco1, preco2, par->acao1_id, par->acao2_id,
                                           &acao_compra, &acao_venda, &preco_compra, &preco_venda)) {
                
                // Calcular volume disponível (mínimo entre as duas ações)
                int volume_disponivel = 1000; // Volume padrão para arbitragem
                
                // Calcular lucro potencial
                double lucro_potencial = calcular_lucro_potencial(preco_compra, preco_venda, volume_disponivel);
                
                // Criar oportunidade
                if (num_oportunidades < MAX_OPORTUNIDADES) {
                    OportunidadeArbitragem* op = &oportunidades[num_oportunidades];
                    op->acao_compra_id = acao_compra;
                    op->acao_venda_id = acao_venda;
                    op->preco_compra = preco_compra;
                    op->preco_venda = preco_venda;
                    op->spread_percentual = spread * 100.0;
                    op->lucro_potencial = lucro_potencial;
                    op->volume_disponivel = volume_disponivel;
                    op->timestamp = time(NULL);
                    op->executada = 0;
                    op->lucro_realizado = 0.0;
                    
                    num_oportunidades++;
                    
                    // Atualizar estatísticas
                    pthread_mutex_lock(&estatisticas_arbitragem.mutex);
                    estatisticas_arbitragem.total_oportunidades_detectadas++;
                    estatisticas_arbitragem.lucro_total_potencial += lucro_potencial;
                    
                    if (spread > estatisticas_arbitragem.maior_spread_detectado) {
                        estatisticas_arbitragem.maior_spread_detectado = spread;
                    }
                    
                    // Contar por setor
                    for (int j = 0; j < 10; j++) {
                        if (strcmp(par->setor, sistema->acoes[acao_compra].setor) == 0) {
                            estatisticas_arbitragem.oportunidades_por_setor[j]++;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&estatisticas_arbitragem.mutex);
                    
                    printf("🚀 OPORTUNIDADE DE ARBITRAGEM DETECTADA!\n");
                    printf("   Compra: %s a R$ %.2f\n", sistema->acoes[acao_compra].nome, preco_compra);
                    printf("   Venda: %s a R$ %.2f\n", sistema->acoes[acao_venda].nome, preco_venda);
                    printf("   Spread: %.2f%%\n", spread * 100.0);
                    printf("   Lucro potencial: R$ %.2f\n", lucro_potencial);
                    printf("   Volume: %d ações\n", volume_disponivel);
                }
            }
        }
    }
}

// Função para executar arbitragem
void executar_arbitragem_detector(TradingSystem* sistema, void* oportunidade_void) {
    OportunidadeArbitragem* oportunidade = (OportunidadeArbitragem*)oportunidade_void;
    printf("💰 EXECUTANDO ARBITRAGEM!\n");
    printf("   Comprando %d ações de %s a R$ %.2f\n", 
           oportunidade->volume_disponivel, 
           sistema->acoes[oportunidade->acao_compra_id].nome, 
           oportunidade->preco_compra);
    
    printf("   Vendendo %d ações de %s a R$ %.2f\n", 
           oportunidade->volume_disponivel, 
           sistema->acoes[oportunidade->acao_venda_id].nome, 
           oportunidade->preco_venda);
    
    // Simular execução das ordens
    pthread_mutex_lock(&sistema->acoes[oportunidade->acao_compra_id].mutex);
    pthread_mutex_lock(&sistema->acoes[oportunidade->acao_venda_id].mutex);
    
    // Atualizar preços (simular impacto da arbitragem)
    double novo_preco_compra = oportunidade->preco_compra * 1.001; // Pequeno aumento
    double novo_preco_venda = oportunidade->preco_venda * 0.999;   // Pequena diminuição
    
    sistema->acoes[oportunidade->acao_compra_id].preco_atual = novo_preco_compra;
    sistema->acoes[oportunidade->acao_venda_id].preco_atual = novo_preco_venda;
    
    // Calcular lucro realizado (considerando custos de transação)
    double custos_transacao = oportunidade->lucro_potencial * 0.001; // 0.1% de custos
    oportunidade->lucro_realizado = oportunidade->lucro_potencial - custos_transacao;
    
    pthread_mutex_unlock(&sistema->acoes[oportunidade->acao_compra_id].mutex);
    pthread_mutex_unlock(&sistema->acoes[oportunidade->acao_venda_id].mutex);
    
    oportunidade->executada = 1;
    
    // Atualizar estatísticas
    pthread_mutex_lock(&estatisticas_arbitragem.mutex);
    estatisticas_arbitragem.total_arbitragens_executadas++;
    estatisticas_arbitragem.lucro_total_realizado += oportunidade->lucro_realizado;
    
    if (oportunidade->spread_percentual < estatisticas_arbitragem.menor_spread_executado) {
        estatisticas_arbitragem.menor_spread_executado = oportunidade->spread_percentual;
    }
    pthread_mutex_unlock(&estatisticas_arbitragem.mutex);
    
    printf("   ✅ Arbitragem executada com sucesso!\n");
    printf("   Lucro realizado: R$ %.2f (após custos)\n", oportunidade->lucro_realizado);
    printf("   Novos preços: %.2f / %.2f\n", novo_preco_compra, novo_preco_venda);
}

// Função para processar oportunidades pendentes
void processar_oportunidades_pendentes(TradingSystem* sistema) {
    for (int i = 0; i < num_oportunidades; i++) {
        OportunidadeArbitragem* op = &oportunidades[i];
        
        if (!op->executada) {
            // Verificar se ainda é uma oportunidade válida
            double preco_compra_atual = sistema->acoes[op->acao_compra_id].preco_atual;
            double preco_venda_atual = sistema->acoes[op->acao_venda_id].preco_atual;
            double spread_atual = calcular_spread(preco_compra_atual, preco_venda_atual);
            
            // Se spread ainda é atrativo, executar
            if (spread_atual > 0.02) { // 2% mínimo
                executar_arbitragem_detector(sistema, op);
            } else {
                printf("⚠️  Oportunidade %d expirou (spread atual: %.2f%%)\n", 
                       i, spread_atual * 100.0);
                op->executada = 1; // Marcar como expirada
            }
        }
    }
}

// Função para exibir estatísticas de arbitragem
void exibir_estatisticas_arbitragem() {
    pthread_mutex_lock(&estatisticas_arbitragem.mutex);
    
    printf("\n=== ESTATÍSTICAS DE ARBITRAGEM ===\n");
    printf("Total de oportunidades detectadas: %d\n", estatisticas_arbitragem.total_oportunidades_detectadas);
    printf("Total de arbitragens executadas: %d\n", estatisticas_arbitragem.total_arbitragens_executadas);
    printf("Taxa de execução: %.1f%%\n", 
           estatisticas_arbitragem.total_oportunidades_detectadas > 0 ? 
           (double)estatisticas_arbitragem.total_arbitragens_executadas / 
           estatisticas_arbitragem.total_oportunidades_detectadas * 100.0 : 0.0);
    printf("Lucro total potencial: R$ %.2f\n", estatisticas_arbitragem.lucro_total_potencial);
    printf("Lucro total realizado: R$ %.2f\n", estatisticas_arbitragem.lucro_total_realizado);
    printf("Eficiência: %.1f%%\n", 
           estatisticas_arbitragem.lucro_total_potencial > 0 ? 
           estatisticas_arbitragem.lucro_total_realizado / 
           estatisticas_arbitragem.lucro_total_potencial * 100.0 : 0.0);
    printf("Maior spread detectado: %.2f%%\n", estatisticas_arbitragem.maior_spread_detectado * 100.0);
    printf("Menor spread executado: %.2f%%\n", estatisticas_arbitragem.menor_spread_executado);
    
    printf("\nOportunidades por setor:\n");
    for (int i = 0; i < 10; i++) {
        if (estatisticas_arbitragem.oportunidades_por_setor[i] > 0) {
            printf("  Setor %d: %d oportunidades\n", i, estatisticas_arbitragem.oportunidades_por_setor[i]);
        }
    }
    
    pthread_mutex_unlock(&estatisticas_arbitragem.mutex);
}

// Função para exibir oportunidades ativas
void exibir_oportunidades_ativas() {
    printf("\n=== OPORTUNIDADES DE ARBITRAGEM ATIVAS ===\n");
    
    int oportunidades_ativas = 0;
    for (int i = 0; i < num_oportunidades; i++) {
        if (!oportunidades[i].executada) {
            oportunidades_ativas++;
            printf("Oportunidade %d:\n", i);
            printf("  Compra: Ação %d a R$ %.2f\n", 
                   oportunidades[i].acao_compra_id, oportunidades[i].preco_compra);
            printf("  Venda: Ação %d a R$ %.2f\n", 
                   oportunidades[i].acao_venda_id, oportunidades[i].preco_venda);
            printf("  Spread: %.2f%%\n", oportunidades[i].spread_percentual);
            printf("  Lucro potencial: R$ %.2f\n", oportunidades[i].lucro_potencial);
            printf("  Volume: %d ações\n", oportunidades[i].volume_disponivel);
        }
    }
    
    if (oportunidades_ativas == 0) {
        printf("Nenhuma oportunidade ativa no momento.\n");
    }
}

// Função principal da thread de arbitragem
void* thread_arbitragem_detector(void* arg) {
    TradingSystem* sistema = (TradingSystem*)arg;
    
    printf("🚀 THREAD DETECTOR DE ARBITRAGEM INICIADA\n");
    printf("Monitorando %ld pares de ações relacionadas...\n", 
           sizeof(pares_relacionadas) / sizeof(ParAcoesRelacionadas) - 1);
    
    // Inicializar estatísticas
    inicializar_estatisticas_arbitragem();
    
    int ciclo = 0;
    while (arbitragem_ativa && sistema->sistema_ativo) {
        ciclo++;
        
        printf("\n--- CICLO DE ARBITRAGEM %d ---\n", ciclo);
        
        // Detectar novas oportunidades
        detectar_oportunidades_arbitragem(sistema);
        
        // Processar oportunidades pendentes
        processar_oportunidades_pendentes(sistema);
        
        // Exibir estatísticas a cada 5 ciclos
        if (ciclo % 5 == 0) {
            exibir_estatisticas_arbitragem();
        }
        
        // Exibir oportunidades ativas a cada 3 ciclos
        if (ciclo % 3 == 0) {
            exibir_oportunidades_ativas();
        }
        
        // Aguardar antes do próximo ciclo
        sleep(3); // 3 segundos entre ciclos
    }
    
    printf("✅ THREAD DETECTOR DE ARBITRAGEM FINALIZADA\n");
    
    // Exibir estatísticas finais
    exibir_estatisticas_arbitragem();
    
    return NULL;
}

// Função para criar thread de arbitragem
int criar_thread_arbitragem_detector(TradingSystem* sistema) {
    pthread_t thread_arbitragem;
    
    int resultado = pthread_create(&thread_arbitragem, NULL, thread_arbitragem_detector, sistema);
    
    if (resultado != 0) {
        printf("❌ Erro ao criar thread detector de arbitragem: %s\n", strerror(resultado));
        return 0;
    }
    
    printf("✅ Thread detector de arbitragem criada com sucesso\n");
    return 1;
}

// Função para parar detector de arbitragem
void parar_detector_arbitragem() {
    arbitragem_ativa = 0;
    printf("🛑 Sinal de parada enviado para detector de arbitragem\n");
}

// Função para obter estatísticas de arbitragem
EstatisticasArbitragem* obter_estatisticas_arbitragem() {
    return &estatisticas_arbitragem;
}

// Função para obter oportunidades de arbitragem
OportunidadeArbitragem* obter_oportunidades_arbitragem(int* num) {
    *num = num_oportunidades;
    return oportunidades;
} 