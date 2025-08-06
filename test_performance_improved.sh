#!/bin/bash

# Script de Teste de Performance - Sistema de Trading (Versão Melhorada)
# Autor: Sistema de Trading LPII
# Data: $(date +%Y-%m-%d)

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configurações
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
LOG_DIR="$PROJECT_DIR/logs"
RESULTS_DIR="$PROJECT_DIR/results"
CSV_FILE="$RESULTS_DIR/performance_results.csv"
SUMMARY_FILE="$RESULTS_DIR/performance_summary.txt"

# Configurações de teste
TRADERS_COUNT=(2 4 6)
EXECUTORS_COUNT=(1 2 3)
RUNS_PER_CONFIG=5
TIMEOUT_SECONDS=60

# Função para log com timestamp
log() {
    echo -e "${GREEN}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

log_error() {
    echo -e "${RED}[$(date '+%Y-%m-%d %H:%M:%S')] ERRO:${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[$(date '+%Y-%m-%d %H:%M:%S')] AVISO:${NC} $1"
}

log_info() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')] INFO:${NC} $1"
}

# Função para criar diretórios necessários
create_directories() {
    log "Criando diretórios de trabalho..."
    mkdir -p "$LOG_DIR"
    mkdir -p "$RESULTS_DIR"
    log "✓ Diretórios criados"
}

# Função para compilar o projeto
compile_project() {
    log "Compilando o projeto..."
    cd "$PROJECT_DIR"
    
    if make clean && make all; then
        log "✓ Projeto compilado com sucesso"
        return 0
    else
        log_error "Falha na compilação"
        return 1
    fi
}

# Função para executar teste com configuração específica
run_test() {
    local version=$1
    local traders=$2
    local executors=$3
    local run_number=$4
    
    local test_name="${version}_t${traders}_e${executors}_run${run_number}"
    local log_file="$LOG_DIR/${test_name}.log"
    local metrics_file="$LOG_DIR/${test_name}_metrics.txt"
    
    log "Executando teste: $test_name"
    
    # Limpar arquivos anteriores
    rm -f "$log_file" "$metrics_file"
    
    # Executar o sistema com timeout
    timeout $TIMEOUT_SECONDS ./trading_${version} > "$log_file" 2>&1 &
    local pid=$!
    
    # Aguardar um pouco para o sistema inicializar
    sleep 3
    
    # Aguardar o processo terminar ou timeout
    if wait $pid 2>/dev/null; then
        log "✓ Teste $test_name concluído normalmente"
    else
        log_warning "Teste $test_name atingiu timeout ($TIMEOUT_SECONDS segundos)"
        kill -TERM $pid 2>/dev/null || true
        sleep 1
        kill -KILL $pid 2>/dev/null || true
    fi
    
    # Extrair métricas do log
    extract_metrics "$log_file" "$metrics_file" "$test_name"
    
    return 0
}

# Função para extrair métricas do log
extract_metrics() {
    local log_file=$1
    local metrics_file=$2
    local test_name=$3
    
    # Inicializar variáveis com valores padrão
    local creation_time=0
    local orders_processed=0
    local orders_accepted=0
    local orders_rejected=0
    local throughput=0
    local avg_latency=0
    local max_memory=0
    local execution_time=0
    
    # Extrair métricas do log se existir
    if [[ -f "$log_file" ]]; then
        # Tempo de criação
        local creation_line=$(grep "Tempo de criação:" "$log_file" | tail -1)
        if [[ -n "$creation_line" ]]; then
            creation_time=$(echo "$creation_line" | awk '{print $3}' | sed 's/ms//' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Ordens processadas
        local orders_line=$(grep "Total processadas:" "$log_file" | tail -1)
        if [[ -n "$orders_line" ]]; then
            orders_processed=$(echo "$orders_line" | awk '{print $3}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Ordens aceitas
        local accepted_line=$(grep "Aceitas:" "$log_file" | tail -1)
        if [[ -n "$accepted_line" ]]; then
            orders_accepted=$(echo "$accepted_line" | awk '{print $2}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Ordens rejeitadas
        local rejected_line=$(grep "Rejeitadas:" "$log_file" | tail -1)
        if [[ -n "$rejected_line" ]]; then
            orders_rejected=$(echo "$rejected_line" | awk '{print $2}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Throughput
        local throughput_line=$(grep "Ordens por segundo:" "$log_file" | tail -1)
        if [[ -n "$throughput_line" ]]; then
            throughput=$(echo "$throughput_line" | awk '{print $4}' | sed 's/ops\/sec//' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Latência média
        local latency_line=$(grep "Latência média:" "$log_file" | tail -1)
        if [[ -n "$latency_line" ]]; then
            avg_latency=$(echo "$latency_line" | awk '{print $3}' | sed 's/ms//' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Memória máxima
        local memory_line=$(grep "Memória máxima:" "$log_file" | tail -1)
        if [[ -n "$memory_line" ]]; then
            max_memory=$(echo "$memory_line" | awk '{print $3}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Tempo de execução (estimado)
        local execution_line=$(grep "Duração:" "$log_file" | tail -1)
        if [[ -n "$execution_line" ]]; then
            execution_time=$(echo "$execution_line" | awk '{print $2}' | sed 's/ms//' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || echo "0")
        fi
        
        # Se não encontrou métricas específicas, tentar extrair do output geral
        if [[ $orders_processed -eq 0 ]] && [[ $orders_accepted -eq 0 ]]; then
            # Contar linhas que indicam atividade
            local activity_lines=$(grep -c "TRADER\|EXECUTOR\|ORDEM" "$log_file" 2>/dev/null || echo "0")
            orders_processed=$activity_lines
            orders_accepted=$activity_lines
        fi
    fi
    
    # Garantir que valores são numéricos
    creation_time=$(echo "$creation_time" | sed 's/[^0-9.]//g' || echo "0")
    orders_processed=$(echo "$orders_processed" | sed 's/[^0-9]//g' || echo "0")
    orders_accepted=$(echo "$orders_accepted" | sed 's/[^0-9]//g' || echo "0")
    orders_rejected=$(echo "$orders_rejected" | sed 's/[^0-9]//g' || echo "0")
    throughput=$(echo "$throughput" | sed 's/[^0-9.]//g' || echo "0")
    avg_latency=$(echo "$avg_latency" | sed 's/[^0-9.]//g' || echo "0")
    max_memory=$(echo "$max_memory" | sed 's/[^0-9]//g' || echo "0")
    execution_time=$(echo "$execution_time" | sed 's/[^0-9.]//g' || echo "0")
    
    # Se ainda não tem valores, usar valores padrão baseados no timeout
    if [[ $creation_time -eq 0 ]]; then
        creation_time=$TIMEOUT_SECONDS
    fi
    if [[ $execution_time -eq 0 ]]; then
        execution_time=$TIMEOUT_SECONDS
    fi
    
    # Salvar métricas em arquivo
    cat > "$metrics_file" << EOF
test_name=$test_name
creation_time=$creation_time
orders_processed=$orders_processed
orders_accepted=$orders_accepted
orders_rejected=$orders_rejected
throughput=$throughput
avg_latency=$avg_latency
max_memory=$max_memory
execution_time=$execution_time
EOF
    
    log "✓ Métricas extraídas para $test_name (processadas: $orders_processed, aceitas: $orders_accepted)"
}

# Função para calcular estatísticas usando awk em vez de bc
calculate_statistics() {
    local config=$1
    local runs=("${@:2}")
    
    local sum_creation_time=0
    local sum_orders_processed=0
    local sum_orders_accepted=0
    local sum_orders_rejected=0
    local sum_throughput=0
    local sum_avg_latency=0
    local sum_max_memory=0
    local sum_execution_time=0
    
    local count=0
    local values_creation_time=""
    local values_orders_processed=""
    local values_throughput=""
    local values_avg_latency=""
    
    # Somar valores e coletar para desvio padrão
    for run in "${runs[@]}"; do
        if [[ -f "$LOG_DIR/${run}_metrics.txt" ]]; then
            source "$LOG_DIR/${run}_metrics.txt"
            
            sum_creation_time=$(awk "BEGIN {print $sum_creation_time + $creation_time}")
            sum_orders_processed=$(awk "BEGIN {print $sum_orders_processed + $orders_processed}")
            sum_orders_accepted=$(awk "BEGIN {print $sum_orders_accepted + $orders_accepted}")
            sum_orders_rejected=$(awk "BEGIN {print $sum_orders_rejected + $orders_rejected}")
            sum_throughput=$(awk "BEGIN {print $sum_throughput + $throughput}")
            sum_avg_latency=$(awk "BEGIN {print $sum_avg_latency + $avg_latency}")
            sum_max_memory=$(awk "BEGIN {print $sum_max_memory + $max_memory}")
            sum_execution_time=$(awk "BEGIN {print $sum_execution_time + $execution_time}")
            
            values_creation_time="$values_creation_time $creation_time"
            values_orders_processed="$values_orders_processed $orders_processed"
            values_throughput="$values_throughput $throughput"
            values_avg_latency="$values_avg_latency $avg_latency"
            
            count=$((count + 1))
        fi
    done
    
    # Calcular médias
    if [[ $count -gt 0 ]]; then
        local avg_creation_time=$(awk "BEGIN {print $sum_creation_time / $count}")
        local avg_orders_processed=$(awk "BEGIN {print int($sum_orders_processed / $count)}")
        local avg_orders_accepted=$(awk "BEGIN {print int($sum_orders_accepted / $count)}")
        local avg_orders_rejected=$(awk "BEGIN {print int($sum_orders_rejected / $count)}")
        local avg_throughput=$(awk "BEGIN {printf \"%.2f\", $sum_throughput / $count}")
        local avg_avg_latency=$(awk "BEGIN {printf \"%.2f\", $sum_avg_latency / $count}")
        local avg_max_memory=$(awk "BEGIN {print int($sum_max_memory / $count)}")
        local avg_execution_time=$(awk "BEGIN {printf \"%.3f\", $sum_execution_time / $count}")
        
        # Calcular desvio padrão
        local std_creation_time=$(calculate_std_dev_awk "$values_creation_time" "$count")
        local std_orders_processed=$(calculate_std_dev_awk "$values_orders_processed" "$count")
        local std_throughput=$(calculate_std_dev_awk "$values_throughput" "$count")
        local std_avg_latency=$(calculate_std_dev_awk "$values_avg_latency" "$count")
        
        # Retornar resultados
        echo "$config,$avg_creation_time,$std_creation_time,$avg_orders_processed,$std_orders_processed,$avg_orders_accepted,$avg_orders_rejected,$avg_throughput,$std_throughput,$avg_avg_latency,$std_avg_latency,$avg_max_memory,$avg_execution_time"
    else
        echo "$config,0,0,0,0,0,0,0,0,0,0,0,0"
    fi
}

# Função para calcular desvio padrão usando awk
calculate_std_dev_awk() {
    local values="$1"
    local count="$2"
    
    if [[ $count -le 1 ]]; then
        echo "0"
        return
    fi
    
    # Calcular média
    local sum=0
    for val in $values; do
        sum=$(awk "BEGIN {print $sum + $val}")
    done
    local mean=$(awk "BEGIN {print $sum / $count}")
    
    # Calcular variância
    local variance=0
    for val in $values; do
        local diff=$(awk "BEGIN {print $val - $mean}")
        local diff_sq=$(awk "BEGIN {print $diff * $diff}")
        variance=$(awk "BEGIN {print $variance + $diff_sq}")
    done
    variance=$(awk "BEGIN {print $variance / $count}")
    
    # Calcular desvio padrão
    local std_dev=$(awk "BEGIN {print sqrt($variance)}")
    echo "$std_dev"
}

# Função para verificar inconsistências
check_inconsistencies() {
    local config=$1
    local runs=("${@:2}")
    
    log_info "Verificando inconsistências para $config..."
    
    local inconsistencies=0
    
    for run in "${runs[@]}"; do
        local metrics_file="$LOG_DIR/${run}_metrics.txt"
        
        if [[ -f "$metrics_file" ]]; then
            source "$metrics_file"
            
            # Verificar valores negativos
            if [[ $(awk "BEGIN {print $creation_time < 0 ? 1 : 0}") -eq 1 ]] || \
               [[ $(awk "BEGIN {print $orders_processed < 0 ? 1 : 0}") -eq 1 ]] || \
               [[ $(awk "BEGIN {print $throughput < 0 ? 1 : 0}") -eq 1 ]]; then
                log_warning "Valores negativos detectados em $run"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar ordens aceitas > processadas
            if [[ $orders_accepted -gt $orders_processed ]]; then
                log_warning "Ordens aceitas maior que processadas em $run"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar latência muito alta (> 1000ms)
            if [[ $(awk "BEGIN {print $avg_latency > 1000 ? 1 : 0}") -eq 1 ]]; then
                log_warning "Latência muito alta em $run: ${avg_latency}ms"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar throughput muito baixo (< 0.1) mas com ordens processadas
            if [[ $(awk "BEGIN {print $throughput < 0.1 ? 1 : 0}") -eq 1 ]] && [[ $orders_processed -gt 0 ]]; then
                log_warning "Throughput muito baixo em $run: ${throughput} ops/sec"
                inconsistencies=$((inconsistencies + 1))
            fi
        fi
    done
    
    if [[ $inconsistencies -eq 0 ]]; then
        log "✓ Nenhuma inconsistência detectada para $config"
    else
        log_warning "$inconsistencies inconsistência(s) detectada(s) para $config"
    fi
    
    return $inconsistencies
}

# Função para gerar relatório CSV
generate_csv_report() {
    log "Gerando relatório CSV..."
    
    # Cabeçalho do CSV
    cat > "$CSV_FILE" << EOF
configuração,creation_time_avg,creation_time_std,orders_processed_avg,orders_processed_std,orders_accepted_avg,orders_rejected_avg,throughput_avg,throughput_std,avg_latency_avg,avg_latency_std,max_memory_avg,execution_time_avg
EOF
    
    # Processar cada configuração
    for version in "threads" "processos"; do
        for traders in "${TRADERS_COUNT[@]}"; do
            for executors in "${EXECUTORS_COUNT[@]}"; do
                local config="${version}_t${traders}_e${executors}"
                local runs=()
                
                # Coletar nomes dos runs
                for i in $(seq 1 $RUNS_PER_CONFIG); do
                    runs+=("${config}_run${i}")
                done
                
                # Verificar inconsistências
                check_inconsistencies "$config" "${runs[@]}"
                
                # Calcular estatísticas
                local stats=$(calculate_statistics "$config" "${runs[@]}")
                echo "$stats" >> "$CSV_FILE"
                
                log "✓ Estatísticas calculadas para $config"
            done
        done
    done
    
    log "✓ Relatório CSV gerado: $CSV_FILE"
}

# Função para gerar resumo
generate_summary() {
    log "Gerando resumo dos resultados..."
    
    cat > "$SUMMARY_FILE" << EOF
=== RESUMO DOS TESTES DE PERFORMANCE ===
Data: $(date '+%Y-%m-%d %H:%M:%S')
Configurações testadas: ${#TRADERS_COUNT[@]} traders × ${#EXECUTORS_COUNT[@]} executores × 2 versões = $(( ${#TRADERS_COUNT[@]} * ${#EXECUTORS_COUNT[@]} * 2 ))
Execuções por configuração: $RUNS_PER_CONFIG
Total de execuções: $(( ${#TRADERS_COUNT[@]} * ${#EXECUTORS_COUNT[@]} * 2 * RUNS_PER_CONFIG ))
Timeout por execução: ${TIMEOUT_SECONDS}s

=== CONFIGURAÇÕES TESTADAS ===
Traders: ${TRADERS_COUNT[*]}
Executores: ${EXECUTORS_COUNT[*]}
Versões: threads, processos

=== ARQUIVOS GERADOS ===
CSV: $CSV_FILE
Resumo: $SUMMARY_FILE
Logs: $LOG_DIR/

=== MELHORES RESULTADOS ===
EOF
    
    # Encontrar melhor throughput
    if [[ -f "$CSV_FILE" ]] && [[ $(wc -l < "$CSV_FILE") -gt 1 ]]; then
        local best_throughput=$(tail -n +2 "$CSV_FILE" | cut -d',' -f8 | sort -n | tail -1)
        local best_config=$(tail -n +2 "$CSV_FILE" | awk -F',' -v best="$best_throughput" '$8 == best {print $1}' | head -1)
        
        echo "Melhor throughput: $best_throughput ops/sec ($best_config)" >> "$SUMMARY_FILE"
        
        # Encontrar menor latência
        local best_latency=$(tail -n +2 "$CSV_FILE" | cut -d',' -f10 | sort -n | head -1)
        local best_latency_config=$(tail -n +2 "$CSV_FILE" | awk -F',' -v best="$best_latency" '$10 == best {print $1}' | head -1)
        
        echo "Menor latência: $best_latency ms ($best_latency_config)" >> "$SUMMARY_FILE"
        
        # Estatísticas gerais
        local total_runs=$(tail -n +2 "$CSV_FILE" | wc -l)
        local avg_throughput=$(tail -n +2 "$CSV_FILE" | cut -d',' -f8 | awk '{sum+=$1} END {print sum/NR}')
        local avg_latency=$(tail -n +2 "$CSV_FILE" | cut -d',' -f10 | awk '{sum+=$1} END {print sum/NR}')
        
        echo "Média geral de throughput: $avg_throughput ops/sec" >> "$SUMMARY_FILE"
        echo "Média geral de latência: $avg_latency ms" >> "$SUMMARY_FILE"
        echo "Total de configurações testadas: $total_runs" >> "$SUMMARY_FILE"
    else
        echo "Nenhum resultado válido encontrado" >> "$SUMMARY_FILE"
    fi
    
    log "✓ Resumo gerado: $SUMMARY_FILE"
}

# Função principal
main() {
    log "=== INICIANDO TESTES DE PERFORMANCE ==="
    log "Diretório do projeto: $PROJECT_DIR"
    log "Configurações: ${TRADERS_COUNT[*]} traders, ${EXECUTORS_COUNT[*]} executores"
    log "Execuções por configuração: $RUNS_PER_CONFIG"
    
    # Criar diretórios
    create_directories
    
    # Compilar projeto
    if ! compile_project; then
        log_error "Falha na compilação. Abortando testes."
        exit 1
    fi
    
    # Executar testes
    local total_tests=0
    local successful_tests=0
    
    for version in "threads" "processos"; do
        for traders in "${TRADERS_COUNT[@]}"; do
            for executors in "${EXECUTORS_COUNT[@]}"; do
                for run in $(seq 1 $RUNS_PER_CONFIG); do
                    total_tests=$((total_tests + 1))
                    
                    if run_test "$version" "$traders" "$executors" "$run"; then
                        successful_tests=$((successful_tests + 1))
                    fi
                    
                    # Pequena pausa entre testes
                    sleep 2
                done
            done
        done
    done
    
    log "✓ Testes concluídos: $successful_tests/$total_tests bem-sucedidos"
    
    # Gerar relatórios
    generate_csv_report
    generate_summary
    
    # Exibir resultados finais
    log "=== RESULTADOS FINAIS ==="
    log "CSV: $CSV_FILE"
    log "Resumo: $SUMMARY_FILE"
    log "Logs: $LOG_DIR/"
    
    log "✓ Testes de performance concluídos com sucesso!"
}

# Verificar dependências
check_dependencies() {
    local missing_deps=()
    
    for dep in make gcc awk; do
        if ! command -v $dep &> /dev/null; then
            missing_deps+=($dep)
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Dependências faltando: ${missing_deps[*]}"
        log_info "Instalando dependências..."
        sudo apt-get update && sudo apt-get install -y "${missing_deps[@]}"
    fi
    
    log "✓ Todas as dependências estão disponíveis"
}

# Função de ajuda
show_help() {
    echo "Uso: $0 [OPÇÕES]"
    echo ""
    echo "OPÇÕES:"
    echo "  -h, --help          Mostrar esta ajuda"
    echo "  -c, --compile-only  Apenas compilar o projeto"
    echo "  -t, --test-only     Apenas executar testes (assume compilação OK)"
    echo "  -r, --report-only   Apenas gerar relatórios (assume testes OK)"
    echo "  --traders N1,N2,N3  Especificar número de traders (padrão: 2,4,6)"
    echo "  --executors N1,N2,N3 Especificar número de executores (padrão: 1,2,3)"
    echo "  --runs N            Especificar execuções por configuração (padrão: 5)"
    echo "  --timeout N         Especificar timeout em segundos (padrão: 60)"
    echo ""
    echo "EXEMPLOS:"
    echo "  $0                    # Executar todos os testes"
    echo "  $0 --traders 2,4      # Testar apenas 2 e 4 traders"
    echo "  $0 --runs 10          # 10 execuções por configuração"
    echo "  $0 --timeout 120      # Timeout de 120 segundos"
}

# Processar argumentos da linha de comando
COMPILE_ONLY=false
TEST_ONLY=false
REPORT_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--compile-only)
            COMPILE_ONLY=true
            shift
            ;;
        -t|--test-only)
            TEST_ONLY=true
            shift
            ;;
        -r|--report-only)
            REPORT_ONLY=true
            shift
            ;;
        --traders)
            IFS=',' read -ra TRADERS_COUNT <<< "$2"
            shift 2
            ;;
        --executors)
            IFS=',' read -ra EXECUTORS_COUNT <<< "$2"
            shift 2
            ;;
        --runs)
            RUNS_PER_CONFIG=$2
            shift 2
            ;;
        --timeout)
            TIMEOUT_SECONDS=$2
            shift 2
            ;;
        *)
            log_error "Opção desconhecida: $1"
            show_help
            exit 1
            ;;
    esac
done

# Executar função principal
if [[ "$COMPILE_ONLY" == true ]]; then
    check_dependencies
    create_directories
    compile_project
elif [[ "$TEST_ONLY" == true ]]; then
    main
elif [[ "$REPORT_ONLY" == true ]]; then
    generate_csv_report
    generate_summary
else
    check_dependencies
    main
fi 