#include "trading_system.h"
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Estrutura para métricas de tempo
typedef struct {
    struct timespec start_time;
    struct timespec end_time;
    double duration_ms;
    double duration_us;
} TimeMetric;

// Estrutura para métricas de recursos
typedef struct {
    long user_time_us;
    long system_time_us;
    long max_rss_kb;
    long page_faults;
    long voluntary_switches;
    long involuntary_switches;
} ResourceMetric;

// Estrutura para métricas de performance
typedef struct {
    TimeMetric creation_time;
    TimeMetric processing_time;
    TimeMetric response_time;
    ResourceMetric resource_usage;
    int orders_processed;
    int orders_accepted;
    int orders_rejected;
    double throughput_ops_per_sec;
    double latency_avg_ms;
    double latency_min_ms;
    double latency_max_ms;
    double latency_std_ms;
    pthread_mutex_t mutex;
} PerformanceMetrics;

// Estrutura para métricas de mercado
typedef struct {
    double volatility;
    double avg_spread;
    double max_spread;
    double min_spread;
    double price_change_rate;
    double volume_change_rate;
    int total_transactions;
    double total_volume;
    double avg_price;
    double max_price;
    double min_price;
} MarketMetrics;

// Estruturas globais para métricas
static PerformanceMetrics process_metrics;
static PerformanceMetrics thread_metrics;
static MarketMetrics market_metrics;
static int metrics_initialized = 0;

// Função para obter timestamp monotônico
void get_monotonic_time(struct timespec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

// Função para calcular diferença de tempo em milissegundos
double calculate_time_diff_ms(struct timespec start, struct timespec end) {
    double start_ms = (double)start.tv_sec * 1000.0 + (double)start.tv_nsec / 1000000.0;
    double end_ms = (double)end.tv_sec * 1000.0 + (double)end.tv_nsec / 1000000.0;
    return end_ms - start_ms;
}

// Função para calcular diferença de tempo em microssegundos
double calculate_time_diff_us(struct timespec start, struct timespec end) {
    double start_us = (double)start.tv_sec * 1000000.0 + (double)start.tv_nsec / 1000.0;
    double end_us = (double)end.tv_sec * 1000000.0 + (double)end.tv_nsec / 1000.0;
    return end_us - start_us;
}

// Função para obter uso de recursos
void get_resource_usage(void* metric_void) {
    ResourceMetric* metric = (ResourceMetric*)metric_void;
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        metric->user_time_us = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
        metric->system_time_us = usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
        metric->max_rss_kb = usage.ru_maxrss;
        metric->page_faults = usage.ru_majflt;
        metric->voluntary_switches = usage.ru_nvcsw;
        metric->involuntary_switches = usage.ru_nivcsw;
    }
}

// Função para inicializar métricas de performance
void inicializar_metricas_performance() {
    if (metrics_initialized) return;
    
    printf("=== INICIALIZANDO MÉTRICAS DE PERFORMANCE ===\n");
    
    // Inicializar métricas de processos
    memset(&process_metrics, 0, sizeof(PerformanceMetrics));
    pthread_mutex_init(&process_metrics.mutex, NULL);
    
    // Inicializar métricas de threads
    memset(&thread_metrics, 0, sizeof(PerformanceMetrics));
    pthread_mutex_init(&thread_metrics.mutex, NULL);
    
    // Inicializar métricas de mercado
    memset(&market_metrics, 0, sizeof(MarketMetrics));
    
    metrics_initialized = 1;
    printf("✓ Métricas de performance inicializadas\n");
}

// Função para iniciar medição de tempo de criação
void iniciar_medicao_criacao(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->creation_time.start_time);
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para finalizar medição de tempo de criação
void finalizar_medicao_criacao(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->creation_time.end_time);
    metrics->creation_time.duration_ms = calculate_time_diff_ms(
        metrics->creation_time.start_time, metrics->creation_time.end_time);
    metrics->creation_time.duration_us = calculate_time_diff_us(
        metrics->creation_time.start_time, metrics->creation_time.end_time);
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para iniciar medição de tempo de processamento
void* iniciar_medicao_processamento(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->processing_time.start_time);
    pthread_mutex_unlock(&metrics->mutex);
    return &metrics->processing_time;
}

// Função para finalizar medição de tempo de processamento
void finalizar_medicao_processamento(int is_process, int order_accepted) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->processing_time.end_time);
    metrics->processing_time.duration_ms = calculate_time_diff_ms(
        metrics->processing_time.start_time, metrics->processing_time.end_time);
    metrics->processing_time.duration_us = calculate_time_diff_us(
        metrics->processing_time.start_time, metrics->processing_time.end_time);
    
    metrics->orders_processed++;
    if (order_accepted) {
        metrics->orders_accepted++;
    } else {
        metrics->orders_rejected++;
    }
    
    // Atualizar estatísticas de latência
    double current_latency = metrics->processing_time.duration_ms;
    if (metrics->orders_processed == 1) {
        metrics->latency_min_ms = current_latency;
        metrics->latency_max_ms = current_latency;
        metrics->latency_avg_ms = current_latency;
    } else {
        if (current_latency < metrics->latency_min_ms) {
            metrics->latency_min_ms = current_latency;
        }
        if (current_latency > metrics->latency_max_ms) {
            metrics->latency_max_ms = current_latency;
        }
        // Média móvel para latência
        metrics->latency_avg_ms = (metrics->latency_avg_ms * (metrics->orders_processed - 1) + current_latency) / metrics->orders_processed;
    }
    
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para iniciar medição de tempo de resposta end-to-end
void* iniciar_medicao_resposta_end_to_end(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->response_time.start_time);
    pthread_mutex_unlock(&metrics->mutex);
    return &metrics->response_time;
}

// Função para finalizar medição de tempo de resposta end-to-end
void finalizar_medicao_resposta_end_to_end(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_monotonic_time(&metrics->response_time.end_time);
    metrics->response_time.duration_ms = calculate_time_diff_ms(
        metrics->response_time.start_time, metrics->response_time.end_time);
    metrics->response_time.duration_us = calculate_time_diff_us(
        metrics->response_time.start_time, metrics->response_time.end_time);
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para coletar estatísticas de recursos
void coletar_estatisticas_recursos(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    get_resource_usage(&metrics->resource_usage);
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para calcular throughput
void calcular_throughput(int is_process, double total_time_seconds) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    if (total_time_seconds > 0) {
        metrics->throughput_ops_per_sec = (double)metrics->orders_processed / total_time_seconds;
    }
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para calcular métricas de mercado
void calcular_metricas_mercado(TradingSystem* sistema) {
    if (!sistema || sistema->num_acoes == 0) return;
    
    double total_volume = 0;
    double total_price = 0;
    double max_price = 0;
    double min_price = DBL_MAX;
    double max_spread = 0;
    double min_spread = DBL_MAX;
    double total_spread = 0;
    int spread_count = 0;
    
    // Calcular estatísticas básicas
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        total_volume += acao->volume_total;
        total_price += acao->preco_atual;
        
        if (acao->preco_atual > max_price) {
            max_price = acao->preco_atual;
        }
        if (acao->preco_atual < min_price) {
            min_price = acao->preco_atual;
        }
        
        // Calcular spreads entre ações relacionadas
        for (int j = i + 1; j < sistema->num_acoes; j++) {
            Acao* acao2 = &sistema->acoes[j];
            double spread = fabs(acao->preco_atual - acao2->preco_atual) / acao->preco_atual * 100.0;
            total_spread += spread;
            spread_count++;
            
            if (spread > max_spread) {
                max_spread = spread;
            }
            if (spread < min_spread) {
                min_spread = spread;
            }
        }
    }
    
    // Atualizar métricas de mercado
    market_metrics.total_volume = total_volume;
    market_metrics.avg_price = total_price / sistema->num_acoes;
    market_metrics.max_price = max_price;
    market_metrics.min_price = min_price;
    market_metrics.avg_spread = spread_count > 0 ? total_spread / spread_count : 0;
    market_metrics.max_spread = max_spread;
    market_metrics.min_spread = min_spread;
    
    // Calcular volatilidade (desvio padrão dos preços)
    double variance = 0;
    for (int i = 0; i < sistema->num_acoes; i++) {
        double diff = sistema->acoes[i].preco_atual - market_metrics.avg_price;
        variance += diff * diff;
    }
    market_metrics.volatility = sqrt(variance / sistema->num_acoes);
    
    // Calcular taxas de mudança (simplificado)
    market_metrics.price_change_rate = (market_metrics.max_price - market_metrics.min_price) / market_metrics.avg_price * 100.0;
    market_metrics.volume_change_rate = total_volume > 0 ? (total_volume / sistema->num_acoes) / 1000.0 : 0;
}

// Função para coletar estatísticas individuais de trader/executor
void coletar_estatisticas_individual(int thread_id, int is_process, int orders_processed, 
                                   double avg_latency, double throughput) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    pthread_mutex_lock(&metrics->mutex);
    
    printf("📊 Estatísticas individuais - %s ID %d:\n", 
           is_process ? "Processo" : "Thread", thread_id);
    printf("   Ordens processadas: %d\n", orders_processed);
    printf("   Latência média: %.2f ms\n", avg_latency);
    printf("   Throughput: %.2f ops/sec\n", throughput);
    
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para exibir métricas de performance
void exibir_metricas_performance(int is_process) {
    PerformanceMetrics* metrics = is_process ? &process_metrics : &thread_metrics;
    const char* type_name = is_process ? "PROCESSOS" : "THREADS";
    
    pthread_mutex_lock(&metrics->mutex);
    
    printf("\n=== MÉTRICAS DE PERFORMANCE - %s ===\n", type_name);
    
    // Tempo de criação
    printf("⏱️  TEMPO DE CRIAÇÃO:\n");
    printf("   Duração: %.3f ms (%.0f μs)\n", 
           metrics->creation_time.duration_ms, metrics->creation_time.duration_us);
    
    // Processamento de ordens
    printf("📈 PROCESSAMENTO DE ORDENS:\n");
    printf("   Total processadas: %d\n", metrics->orders_processed);
    printf("   Aceitas: %d\n", metrics->orders_accepted);
    printf("   Rejeitadas: %d\n", metrics->orders_rejected);
    printf("   Taxa de aceitação: %.1f%%\n", 
           metrics->orders_processed > 0 ? 
           (double)metrics->orders_accepted / metrics->orders_processed * 100.0 : 0.0);
    
    // Latência
    printf("⏳ LATÊNCIA:\n");
    printf("   Média: %.2f ms\n", metrics->latency_avg_ms);
    printf("   Mínima: %.2f ms\n", metrics->latency_min_ms);
    printf("   Máxima: %.2f ms\n", metrics->latency_max_ms);
    
    // Throughput
    printf("🚀 THROUGHPUT:\n");
    printf("   Ordens por segundo: %.2f ops/sec\n", metrics->throughput_ops_per_sec);
    
    // Tempo de resposta end-to-end
    printf("🔄 TEMPO DE RESPOSTA END-TO-END:\n");
    printf("   Duração: %.3f ms (%.0f μs)\n", 
           metrics->response_time.duration_ms, metrics->response_time.duration_us);
    
    // Uso de recursos
    printf("💾 USO DE RECURSOS:\n");
    printf("   Tempo de usuário: %.2f ms\n", metrics->resource_usage.user_time_us / 1000.0);
    printf("   Tempo de sistema: %.2f ms\n", metrics->resource_usage.system_time_us / 1000.0);
    printf("   Memória máxima: %ld KB\n", metrics->resource_usage.max_rss_kb);
    printf("   Page faults: %ld\n", metrics->resource_usage.page_faults);
    printf("   Switches voluntários: %ld\n", metrics->resource_usage.voluntary_switches);
    printf("   Switches involuntários: %ld\n", metrics->resource_usage.involuntary_switches);
    
    pthread_mutex_unlock(&metrics->mutex);
}

// Função para exibir métricas de mercado
void exibir_metricas_mercado() {
    printf("\n=== MÉTRICAS DE MERCADO ===\n");
    
    printf("📊 ESTATÍSTICAS GERAIS:\n");
    printf("   Volume total: %.0f\n", market_metrics.total_volume);
    printf("   Preço médio: %.2f\n", market_metrics.avg_price);
    printf("   Preço máximo: %.2f\n", market_metrics.max_price);
    printf("   Preço mínimo: %.2f\n", market_metrics.min_price);
    
    printf("📈 VOLATILIDADE E SPREAD:\n");
    printf("   Volatilidade: %.4f\n", market_metrics.volatility);
    printf("   Spread médio: %.2f%%\n", market_metrics.avg_spread);
    printf("   Spread máximo: %.2f%%\n", market_metrics.max_spread);
    printf("   Spread mínimo: %.2f%%\n", market_metrics.min_spread);
    
    printf("🔄 TAXAS DE MUDANÇA:\n");
    printf("   Taxa de mudança de preço: %.2f%%\n", market_metrics.price_change_rate);
    printf("   Taxa de mudança de volume: %.2f\n", market_metrics.volume_change_rate);
}

// Função para comparar processos vs threads
void comparar_processos_vs_threads() {
    printf("\n=== COMPARAÇÃO PROCESSOS vs THREADS ===\n");
    
    pthread_mutex_lock(&process_metrics.mutex);
    pthread_mutex_lock(&thread_metrics.mutex);
    
    printf("⏱️  TEMPO DE CRIAÇÃO:\n");
    printf("   Processos: %.3f ms\n", process_metrics.creation_time.duration_ms);
    printf("   Threads: %.3f ms\n", thread_metrics.creation_time.duration_ms);
    printf("   Diferença: %.3f ms (%.1f%%)\n", 
           process_metrics.creation_time.duration_ms - thread_metrics.creation_time.duration_ms,
           thread_metrics.creation_time.duration_ms > 0 ? 
           (process_metrics.creation_time.duration_ms / thread_metrics.creation_time.duration_ms - 1) * 100 : 0);
    
    printf("\n📈 PROCESSAMENTO:\n");
    printf("   Processos - Ordens: %d, Throughput: %.2f ops/sec\n", 
           process_metrics.orders_processed, process_metrics.throughput_ops_per_sec);
    printf("   Threads - Ordens: %d, Throughput: %.2f ops/sec\n", 
           thread_metrics.orders_processed, thread_metrics.throughput_ops_per_sec);
    
    printf("\n⏳ LATÊNCIA MÉDIA:\n");
    printf("   Processos: %.2f ms\n", process_metrics.latency_avg_ms);
    printf("   Threads: %.2f ms\n", thread_metrics.latency_avg_ms);
    
    printf("\n💾 USO DE MEMÓRIA:\n");
    printf("   Processos: %ld KB\n", process_metrics.resource_usage.max_rss_kb);
    printf("   Threads: %ld KB\n", thread_metrics.resource_usage.max_rss_kb);
    
    pthread_mutex_unlock(&thread_metrics.mutex);
    pthread_mutex_unlock(&process_metrics.mutex);
}

// Função para salvar métricas em arquivo
void salvar_metricas_arquivo(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("❌ Erro ao criar arquivo de métricas: %s\n", filename);
        return;
    }
    
    fprintf(file, "=== MÉTRICAS DE PERFORMANCE ===\n");
    fprintf(file, "Data/Hora: %s\n", format_timestamp(time(NULL), 0));
    
    // Métricas de processos
    fprintf(file, "\n--- PROCESSOS ---\n");
    fprintf(file, "Tempo de criação: %.3f ms\n", process_metrics.creation_time.duration_ms);
    fprintf(file, "Ordens processadas: %d\n", process_metrics.orders_processed);
    fprintf(file, "Throughput: %.2f ops/sec\n", process_metrics.throughput_ops_per_sec);
    fprintf(file, "Latência média: %.2f ms\n", process_metrics.latency_avg_ms);
    fprintf(file, "Memória máxima: %ld KB\n", process_metrics.resource_usage.max_rss_kb);
    
    // Métricas de threads
    fprintf(file, "\n--- THREADS ---\n");
    fprintf(file, "Tempo de criação: %.3f ms\n", thread_metrics.creation_time.duration_ms);
    fprintf(file, "Ordens processadas: %d\n", thread_metrics.orders_processed);
    fprintf(file, "Throughput: %.2f ops/sec\n", thread_metrics.throughput_ops_per_sec);
    fprintf(file, "Latência média: %.2f ms\n", thread_metrics.latency_avg_ms);
    fprintf(file, "Memória máxima: %ld KB\n", thread_metrics.resource_usage.max_rss_kb);
    
    // Métricas de mercado
    fprintf(file, "\n--- MERCADO ---\n");
    fprintf(file, "Volatilidade: %.4f\n", market_metrics.volatility);
    fprintf(file, "Spread médio: %.2f%%\n", market_metrics.avg_spread);
    fprintf(file, "Volume total: %.0f\n", market_metrics.total_volume);
    
    fclose(file);
    printf("✓ Métricas salvas em: %s\n", filename);
}

// Função para finalizar métricas
void finalizar_metricas_performance() {
    if (!metrics_initialized) return;
    
    printf("\n=== FINALIZANDO MÉTRICAS DE PERFORMANCE ===\n");
    
    // Exibir métricas finais
    exibir_metricas_performance(1); // Processos
    exibir_metricas_performance(0); // Threads
    exibir_metricas_mercado();
    comparar_processos_vs_threads();
    
    // Salvar em arquivo
    salvar_metricas_arquivo("performance_metrics.txt");
    
    // Limpar recursos
    pthread_mutex_destroy(&process_metrics.mutex);
    pthread_mutex_destroy(&thread_metrics.mutex);
    
    metrics_initialized = 0;
    printf("✓ Métricas de performance finalizadas\n");
}

// Função para obter métricas de performance
void* obter_metricas_processos() {
    return &process_metrics;
}

void* obter_metricas_threads() {
    return &thread_metrics;
}

void* obter_metricas_mercado() {
    return &market_metrics;
} 