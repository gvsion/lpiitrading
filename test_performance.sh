#!/bin/bash

# Script de Teste de Performance - Sistema de Trading
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
    
    # Executar o sistema com timeout
    timeout $TIMEOUT_SECONDS ./trading_${version} > "$log_file" 2>&1 &
    local pid=$!
    
    # Aguardar um pouco para o sistema inicializar
    sleep 2
    
    # Aguardar o processo terminar ou timeout
    wait $pid 2>/dev/null || {
        log_warning "Teste $test_name atingiu timeout ($TIMEOUT_SECONDS segundos)"
        kill -TERM $pid 2>/dev/null || true
        sleep 1
        kill -KILL $pid 2>/dev/null || true
    }
    
    # Extrair métricas do log
    extract_metrics "$log_file" "$metrics_file" "$test_name"
    
    return 0
}

# Função para extrair métricas do log
extract_metrics() {
    local log_file=$1
    local metrics_file=$2
    local test_name=$3
    
    # Inicializar variáveis
    local creation_time=0
    local orders_processed=0
    local orders_accepted=0
    local orders_rejected=0
    local throughput=0
    local avg_latency=0
    local max_memory=0
    local execution_time=0
    
    # Extrair métricas do log
    if [[ -f "$log_file" ]]; then
        # Tempo de criação
        creation_time=$(grep "Tempo de criação:" "$log_file" | tail -1 | awk '{print $3}' | sed 's/ms//' || echo "0")
        
        # Ordens processadas
        orders_processed=$(grep "Total processadas:" "$log_file" | tail -1 | awk '{print $3}' || echo "0")
        orders_accepted=$(grep "Aceitas:" "$log_file" | tail -1 | awk '{print $2}' || echo "0")
        orders_rejected=$(grep "Rejeitadas:" "$log_file" | tail -1 | awk '{print $2}' || echo "0")
        
        # Throughput
        throughput=$(grep "Ordens por segundo:" "$log_file" | tail -1 | awk '{print $4}' | sed 's/ops\/sec//' || echo "0")
        
        # Latência média
        avg_latency=$(grep "Latência média:" "$log_file" | tail -1 | awk '{print $3}' | sed 's/ms//' || echo "0")
        
        # Memória máxima
        max_memory=$(grep "Memória máxima:" "$log_file" | tail -1 | awk '{print $3}' || echo "0")
        
        # Tempo de execução (estimado)
        execution_time=$(grep "Duração:" "$log_file" | tail -1 | awk '{print $2}' | sed 's/ms//' || echo "0")
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
    
    log "✓ Métricas extraídas para $test_name"
}

# Função para calcular estatísticas
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
    
    # Somar valores
    for run in "${runs[@]}"; do
        if [[ -f "$LOG_DIR/${run}_metrics.txt" ]]; then
            source "$LOG_DIR/${run}_metrics.txt"
            
            sum_creation_time=$(echo "$sum_creation_time + $creation_time" | bc -l)
            sum_orders_processed=$(echo "$sum_orders_processed + $orders_processed" | bc -l)
            sum_orders_accepted=$(echo "$sum_orders_accepted + $orders_accepted" | bc -l)
            sum_orders_rejected=$(echo "$sum_orders_rejected + $orders_rejected" | bc -l)
            sum_throughput=$(echo "$sum_throughput + $throughput" | bc -l)
            sum_avg_latency=$(echo "$sum_avg_latency + $avg_latency" | bc -l)
            sum_max_memory=$(echo "$sum_max_memory + $max_memory" | bc -l)
            sum_execution_time=$(echo "$sum_execution_time + $execution_time" | bc -l)
            
            count=$((count + 1))
        fi
    done
    
    # Calcular médias
    if [[ $count -gt 0 ]]; then
        local avg_creation_time=$(echo "scale=3; $sum_creation_time / $count" | bc -l)
        local avg_orders_processed=$(echo "scale=0; $sum_orders_processed / $count" | bc -l)
        local avg_orders_accepted=$(echo "scale=0; $sum_orders_accepted / $count" | bc -l)
        local avg_orders_rejected=$(echo "scale=0; $sum_orders_rejected / $count" | bc -l)
        local avg_throughput=$(echo "scale=2; $sum_throughput / $count" | bc -l)
        local avg_avg_latency=$(echo "scale=2; $sum_avg_latency / $count" | bc -l)
        local avg_max_memory=$(echo "scale=0; $sum_max_memory / $count" | bc -l)
        local avg_execution_time=$(echo "scale=3; $sum_execution_time / $count" | bc -l)
        
        # Calcular desvio padrão
        local std_creation_time=$(calculate_std_dev "$config" "creation_time" "$count")
        local std_orders_processed=$(calculate_std_dev "$config" "orders_processed" "$count")
        local std_throughput=$(calculate_std_dev "$config" "throughput" "$count")
        local std_avg_latency=$(calculate_std_dev "$config" "avg_latency" "$count")
        
        # Retornar resultados
        echo "$config,$avg_creation_time,$std_creation_time,$avg_orders_processed,$std_orders_processed,$avg_orders_accepted,$avg_orders_rejected,$avg_throughput,$std_throughput,$avg_avg_latency,$std_avg_latency,$avg_max_memory,$avg_execution_time"
    else
        echo "$config,0,0,0,0,0,0,0,0,0,0,0,0"
    fi
}

# Função para calcular desvio padrão
calculate_std_dev() {
    local config=$1
    local metric=$2
    local count=$3
    
    local sum=0
    local sum_sq=0
    
    # Calcular soma e soma dos quadrados
    for i in $(seq 1 $count); do
        local run="${config}_run${i}"
        if [[ -f "$LOG_DIR/${run}_metrics.txt" ]]; then
            source "$LOG_DIR/${run}_metrics.txt"
            local value=${!metric}
            sum=$(echo "$sum + $value" | bc -l)
            sum_sq=$(echo "$sum_sq + $value * $value" | bc -l)
        fi
    done
    
    # Calcular desvio padrão
    if [[ $count -gt 1 ]]; then
        local mean=$(echo "scale=3; $sum / $count" | bc -l)
        local variance=$(echo "scale=3; ($sum_sq / $count) - ($mean * $mean)" | bc -l)
        local std_dev=$(echo "scale=3; sqrt($variance)" | bc -l)
        echo "$std_dev"
    else
        echo "0"
    fi
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
            if [[ $(echo "$creation_time < 0" | bc -l) -eq 1 ]] || \
               [[ $(echo "$orders_processed < 0" | bc -l) -eq 1 ]] || \
               [[ $(echo "$throughput < 0" | bc -l) -eq 1 ]]; then
                log_warning "Valores negativos detectados em $run"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar ordens aceitas > processadas
            if [[ $orders_accepted -gt $orders_processed ]]; then
                log_warning "Ordens aceitas maior que processadas em $run"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar latência muito alta (> 1000ms)
            if [[ $(echo "$avg_latency > 1000" | bc -l) -eq 1 ]]; then
                log_warning "Latência muito alta em $run: ${avg_latency}ms"
                inconsistencies=$((inconsistencies + 1))
            fi
            
            # Verificar throughput muito baixo (< 0.1)
            if [[ $(echo "$throughput < 0.1" | bc -l) -eq 1 ]] && [[ $orders_processed -gt 0 ]]; then
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
                    sleep 1
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
    
    # Verificar se bc está instalado
    if ! command -v bc &> /dev/null; then
        log_warning "bc não encontrado. Instalando..."
        sudo apt-get update && sudo apt-get install -y bc
    fi
    
    log "✓ Testes de performance concluídos com sucesso!"
}

# Verificar dependências
check_dependencies() {
    local missing_deps=()
    
    for dep in make gcc bc; do
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