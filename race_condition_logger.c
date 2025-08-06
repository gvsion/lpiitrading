#include "trading_system.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>

// Estrutura para log de operação
typedef struct {
    time_t timestamp;
    long microsec;
    int thread_id;
    char operation_type[50];
    char data_type[50];
    int data_id;
    double old_value;
    double new_value;
    char details[200];
    int is_read;
    int is_write;
    int is_inconsistent;
} LogEntry;

// Estrutura para estado esperado vs observado
typedef struct {
    int acao_id;
    double preco_esperado;
    double preco_observado;
    double diferenca;
    int volume_esperado;
    int volume_observado;
    int volume_diferenca;
    time_t timestamp;
} EstadoComparacao;

// Estrutura para estatísticas de logging
typedef struct {
    int total_operations;
    int read_operations;
    int write_operations;
    int inconsistent_operations;
    int race_conditions_detected;
    double total_execution_time;
    pthread_mutex_t mutex;
} LoggingStats;

// Dados globais para logging
static LogEntry log_entries[MAX_LOG_ENTRIES];
static int log_index = 0;
static LoggingStats logging_stats;
static FILE* log_file = NULL;
static int logging_enabled = 1;
static int execution_run = 0;

// Função para obter timestamp preciso
void get_precise_timestamp(time_t* timestamp, long* microsec) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *timestamp = tv.tv_sec;
    *microsec = tv.tv_usec;
}

// Função para formatar timestamp
char* format_timestamp(time_t timestamp, long microsec) {
    static char buffer[50];
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", tm_info);
    sprintf(buffer + strlen(buffer), ".%06ld", microsec);
    return buffer;
}

// Função para inicializar sistema de logging
void inicializar_race_condition_logger() {
    printf("=== INICIALIZANDO RACE CONDITION LOGGER ===\n");
    
    // Inicializar estatísticas
    logging_stats.total_operations = 0;
    logging_stats.read_operations = 0;
    logging_stats.write_operations = 0;
    logging_stats.inconsistent_operations = 0;
    logging_stats.race_conditions_detected = 0;
    logging_stats.total_execution_time = 0.0;
    pthread_mutex_init(&logging_stats.mutex, NULL);
    
    // Criar arquivo de log
    char filename[100];
    sprintf(filename, "race_condition_log_%d.txt", execution_run);
    log_file = fopen(filename, "w");
    
    if (log_file) {
        fprintf(log_file, "=== RACE CONDITION LOG - EXECUÇÃO %d ===\n", execution_run);
        fprintf(log_file, "Iniciado em: %s\n", format_timestamp(time(NULL), 0));
        fprintf(log_file, "Formato: Timestamp | Thread | Operação | Tipo | ID | Valor_Antigo | Valor_Novo | Detalhes\n");
        fprintf(log_file, "================================================================================\n");
        printf("✓ Arquivo de log criado: %s\n", filename);
    } else {
        printf("❌ Erro ao criar arquivo de log\n");
        logging_enabled = 0;
    }
    
    printf("✓ Sistema de logging inicializado\n");
}

// Função para registrar operação
void log_operation(int thread_id, const char* operation_type, const char* data_type, 
                  int data_id, double old_value, double new_value, const char* details) {
    if (!logging_enabled || log_index >= MAX_LOG_ENTRIES) return;
    
    pthread_mutex_lock(&logging_stats.mutex);
    
    // Obter timestamp preciso
    time_t timestamp;
    long microsec;
    get_precise_timestamp(&timestamp, &microsec);
    
    // Criar entrada de log
    LogEntry* entry = &log_entries[log_index];
    entry->timestamp = timestamp;
    entry->microsec = microsec;
    entry->thread_id = thread_id;
    strncpy(entry->operation_type, operation_type, 49);
    strncpy(entry->data_type, data_type, 49);
    entry->data_id = data_id;
    entry->old_value = old_value;
    entry->new_value = new_value;
    strncpy(entry->details, details, 199);
    
    // Determinar se é leitura ou escrita
    entry->is_read = (strstr(operation_type, "READ") != NULL);
    entry->is_write = (strstr(operation_type, "WRITE") != NULL);
    
    // Detectar inconsistências
    entry->is_inconsistent = 0;
    if (new_value < 0) {
        entry->is_inconsistent = 1;
        strcat(entry->details, " [PREÇO_NEGATIVO]");
    }
    if (data_type[0] == 'V' && new_value < 0) { // Volume
        entry->is_inconsistent = 1;
        strcat(entry->details, " [VOLUME_NEGATIVO]");
    }
    if (fabs(new_value - old_value) > 1000) { // Variação muito grande
        entry->is_inconsistent = 1;
        strcat(entry->details, " [VARIAÇÃO_EXTREMA]");
    }
    
    // Atualizar estatísticas
    logging_stats.total_operations++;
    if (entry->is_read) logging_stats.read_operations++;
    if (entry->is_write) logging_stats.write_operations++;
    if (entry->is_inconsistent) {
        logging_stats.inconsistent_operations++;
        logging_stats.race_conditions_detected++;
    }
    
    // Escrever no arquivo de log
    if (log_file) {
        fprintf(log_file, "%s | Thread_%d | %s | %s | %d | %.2f | %.2f | %s\n",
                format_timestamp(timestamp, microsec),
                thread_id,
                operation_type,
                data_type,
                data_id,
                old_value,
                new_value,
                details);
        fflush(log_file); // Forçar escrita imediata
    }
    
    log_index++;
    pthread_mutex_unlock(&logging_stats.mutex);
}

// Função para detectar race conditions em tempo real
void detectar_race_condition_tempo_real(int thread_id, const char* operation, 
                                       const char* data_type, int data_id, 
                                       double old_value, double new_value) {
    static double last_values[MAX_ACOES] = {0};
    static time_t last_timestamps[MAX_ACOES] = {0};
    static pthread_mutex_t race_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&race_mutex);
    
    // Verificar se houve mudança inesperada
    if (data_id < MAX_ACOES && last_values[data_id] != 0) {
        double expected_change = new_value - last_values[data_id];
        double actual_change = new_value - old_value;
        
        // Se a mudança não corresponde ao esperado, pode ser race condition
        if (fabs(actual_change - expected_change) > 0.01) {
            printf("🚨 RACE CONDITION DETECTADA!\n");
            printf("   Thread: %d\n", thread_id);
            printf("   Operação: %s\n", operation);
            printf("   Dados: %s %d\n", data_type, data_id);
            printf("   Valor esperado: %.2f\n", last_values[data_id]);
            printf("   Valor anterior: %.2f\n", old_value);
            printf("   Valor atual: %.2f\n", new_value);
            printf("   Mudança esperada: %.2f\n", expected_change);
            printf("   Mudança real: %.2f\n", actual_change);
            
            logging_stats.race_conditions_detected++;
        }
    }
    
    last_values[data_id] = new_value;
    last_timestamps[data_id] = time(NULL);
    
    pthread_mutex_unlock(&race_mutex);
}

// Função para comparar estados esperados vs observados
EstadoComparacao* comparar_estados_esperados_observados(TradingSystem* sistema, int* num_comparacoes) {
    static EstadoComparacao comparacoes[MAX_ACOES];
    *num_comparacoes = 0;
    
    for (int i = 0; i < sistema->num_acoes; i++) {
        Acao* acao = &sistema->acoes[i];
        
        // Calcular preço esperado baseado em operações anteriores
        double preco_esperado = acao->preco_atual; // Simplificado
        double preco_observado = acao->preco_atual;
        
        // Calcular volume esperado
        int volume_esperado = acao->volume_total;
        int volume_observado = acao->volume_total;
        
        // Se há diferença significativa, registrar
        if (fabs(preco_observado - preco_esperado) > 0.01 || 
            abs(volume_observado - volume_esperado) > 0) {
            
            EstadoComparacao* comp = &comparacoes[*num_comparacoes];
            comp->acao_id = i;
            comp->preco_esperado = preco_esperado;
            comp->preco_observado = preco_observado;
            comp->diferenca = preco_observado - preco_esperado;
            comp->volume_esperado = volume_esperado;
            comp->volume_observado = volume_observado;
            comp->volume_diferenca = volume_observado - volume_esperado;
            comp->timestamp = time(NULL);
            
            (*num_comparacoes)++;
        }
    }
    
    return comparacoes;
}

// Função para gerar relatório de diferenças entre execuções
void gerar_relatorio_diferencas_execucoes() {
    printf("\n=== RELATÓRIO DE DIFERENÇAS ENTRE EXECUÇÕES ===\n");
    
    pthread_mutex_lock(&logging_stats.mutex);
    
    printf("Estatísticas da Execução %d:\n", execution_run);
    printf("  Total de operações: %d\n", logging_stats.total_operations);
    printf("  Operações de leitura: %d\n", logging_stats.read_operations);
    printf("  Operações de escrita: %d\n", logging_stats.write_operations);
    printf("  Operações inconsistentes: %d\n", logging_stats.inconsistent_operations);
    printf("  Race conditions detectadas: %d\n", logging_stats.race_conditions_detected);
    printf("  Taxa de race conditions: %.2f%%\n", 
           logging_stats.total_operations > 0 ? 
           (double)logging_stats.race_conditions_detected / logging_stats.total_operations * 100.0 : 0.0);
    
    // Analisar logs para padrões
    int race_conditions_por_thread[10] = {0}; // Máximo 10 threads
    int operacoes_por_thread[10] = {0};
    
    for (int i = 0; i < log_index; i++) {
        LogEntry* entry = &log_entries[i];
        if (entry->thread_id < 10) { // Máximo 10 threads
            operacoes_por_thread[entry->thread_id]++;
            if (entry->is_inconsistent) {
                race_conditions_por_thread[entry->thread_id]++;
            }
        }
    }
    
    printf("\nRace Conditions por Thread:\n");
    for (int i = 0; i < 10; i++) {
        if (operacoes_por_thread[i] > 0) {
            printf("  Thread %d: %d/%d (%.1f%%)\n", 
                   i, race_conditions_por_thread[i], operacoes_por_thread[i],
                   (double)race_conditions_por_thread[i] / operacoes_por_thread[i] * 100.0);
        }
    }
    
    pthread_mutex_unlock(&logging_stats.mutex);
}

// Função para analisar logs e detectar padrões
void analisar_padroes_race_conditions() {
    printf("\n=== ANÁLISE DE PADRÕES DE RACE CONDITIONS ===\n");
    
    pthread_mutex_lock(&logging_stats.mutex);
    
    // Contar operações por tipo de dados
    int operacoes_preco = 0, operacoes_volume = 0, operacoes_contador = 0;
    int race_conditions_preco = 0, race_conditions_volume = 0, race_conditions_contador = 0;
    
    for (int i = 0; i < log_index; i++) {
        LogEntry* entry = &log_entries[i];
        
        if (strstr(entry->data_type, "PRECO")) {
            operacoes_preco++;
            if (entry->is_inconsistent) race_conditions_preco++;
        } else if (strstr(entry->data_type, "VOLUME")) {
            operacoes_volume++;
            if (entry->is_inconsistent) race_conditions_volume++;
        } else if (strstr(entry->data_type, "CONTADOR")) {
            operacoes_contador++;
            if (entry->is_inconsistent) race_conditions_contador++;
        }
    }
    
    printf("Race Conditions por Tipo de Dados:\n");
    if (operacoes_preco > 0) {
        printf("  Preços: %d/%d (%.1f%%)\n", 
               race_conditions_preco, operacoes_preco,
               (double)race_conditions_preco / operacoes_preco * 100.0);
    }
    if (operacoes_volume > 0) {
        printf("  Volumes: %d/%d (%.1f%%)\n", 
               race_conditions_volume, operacoes_volume,
               (double)race_conditions_volume / operacoes_volume * 100.0);
    }
    if (operacoes_contador > 0) {
        printf("  Contadores: %d/%d (%.1f%%)\n", 
               race_conditions_contador, operacoes_contador,
               (double)race_conditions_contador / operacoes_contador * 100.0);
    }
    
    // Detectar sequências problemáticas
    printf("\nSequências Problemáticas Detectadas:\n");
    int sequencias_problematicas = 0;
    
    for (int i = 1; i < log_index; i++) {
        LogEntry* prev = &log_entries[i-1];
        LogEntry* curr = &log_entries[i];
        
        // Se duas threads diferentes acessam o mesmo dado em sequência
        if (prev->thread_id != curr->thread_id && 
            prev->data_id == curr->data_id &&
            (curr->timestamp - prev->timestamp) < 1000) { // Menos de 1 segundo
            
            printf("  Thread %d → Thread %d: %s %d (%.2f → %.2f)\n",
                   prev->thread_id, curr->thread_id, curr->data_type, 
                   curr->data_id, prev->new_value, curr->new_value);
            sequencias_problematicas++;
        }
    }
    
    printf("Total de sequências problemáticas: %d\n", sequencias_problematicas);
    
    pthread_mutex_unlock(&logging_stats.mutex);
}

// Função para finalizar logging
void finalizar_race_condition_logger() {
    printf("\n=== FINALIZANDO RACE CONDITION LOGGER ===\n");
    
    pthread_mutex_lock(&logging_stats.mutex);
    
    // Gerar relatório final
    if (log_file) {
        fprintf(log_file, "\n=== RELATÓRIO FINAL ===\n");
        fprintf(log_file, "Total de operações: %d\n", logging_stats.total_operations);
        fprintf(log_file, "Operações de leitura: %d\n", logging_stats.read_operations);
        fprintf(log_file, "Operações de escrita: %d\n", logging_stats.write_operations);
        fprintf(log_file, "Operações inconsistentes: %d\n", logging_stats.inconsistent_operations);
        fprintf(log_file, "Race conditions detectadas: %d\n", logging_stats.race_conditions_detected);
        fprintf(log_file, "Taxa de race conditions: %.2f%%\n", 
               logging_stats.total_operations > 0 ? 
               (double)logging_stats.race_conditions_detected / logging_stats.total_operations * 100.0 : 0.0);
        
        fclose(log_file);
        log_file = NULL;
    }
    
    pthread_mutex_unlock(&logging_stats.mutex);
    pthread_mutex_destroy(&logging_stats.mutex);
    
    printf("✓ Sistema de logging finalizado\n");
}

// Função para executar múltiplas vezes e documentar diferenças
void executar_multiplas_vezes_com_logging(int num_execucoes) {
    printf("\n=== EXECUTANDO %d VEZES COM LOGGING DETALHADO ===\n", num_execucoes);
    
    for (int exec = 1; exec <= num_execucoes; exec++) {
        printf("\n--- EXECUÇÃO %d/%d ---\n", exec, num_execucoes);
        
        // Definir número da execução
        execution_run = exec;
        
        // Inicializar logging
        inicializar_race_condition_logger();
        
        // Executar demo de race conditions
        executar_demo_race_conditions();
        
        // Gerar relatório
        gerar_relatorio_diferencas_execucoes();
        analisar_padroes_race_conditions();
        
        // Finalizar logging
        finalizar_race_condition_logger();
        
        // Pequena pausa entre execuções
        if (exec < num_execucoes) {
            printf("Aguardando 2 segundos antes da próxima execução...\n");
            sleep(2);
        }
    }
    
    printf("\n=== TODAS AS EXECUÇÕES FINALIZADAS ===\n");
    printf("Logs salvos em arquivos: race_condition_log_1.txt até race_condition_log_%d.txt\n", num_execucoes);
}

// Função para comparar arquivos de log
void comparar_arquivos_log(int num_execucoes) {
    printf("\n=== COMPARAÇÃO DE ARQUIVOS DE LOG ===\n");
    
    for (int i = 1; i <= num_execucoes; i++) {
        char filename[100];
        sprintf(filename, "race_condition_log_%d.txt", i);
        
        FILE* file = fopen(filename, "r");
        if (file) {
            printf("Arquivo %s:\n", filename);
            
            char line[500];
            int line_count = 0;
            int race_conditions = 0;
            
            while (fgets(line, sizeof(line), file)) {
                line_count++;
                if (strstr(line, "RACE") || strstr(line, "INCONSISTENT")) {
                    race_conditions++;
                }
            }
            
            printf("  Total de linhas: %d\n", line_count);
            printf("  Race conditions: %d\n", race_conditions);
            printf("  Taxa: %.1f%%\n", 
                   line_count > 0 ? (double)race_conditions / line_count * 100.0 : 0.0);
            
            fclose(file);
        }
    }
}

// Função para obter estatísticas de logging
LoggingStats* obter_estatisticas_logging() {
    return &logging_stats;
}

// Função para verificar se logging está ativo
int logging_esta_ativo() {
    return logging_enabled;
} 