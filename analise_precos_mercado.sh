#!/bin/bash

# Script de Análise de Preços e Eficiência do Mercado
# Autor: Sistema de Trading LPII
# Data: $(date +%Y-%m-%d)

set -e

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configurações
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"
RESULTS_DIR="$SCRIPT_DIR/results"
ANALYSIS_DIR="$SCRIPT_DIR/analysis"

# Função para log
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

# Função para criar diretórios
create_directories() {
    mkdir -p "$ANALYSIS_DIR"
    log "✓ Diretórios de análise criados"
}

# Função para extrair preços iniciais e finais
extract_price_data() {
    local log_file=$1
    local output_file=$2
    
    log_info "Extraindo dados de preços de $log_file"
    
    # Extrair preços iniciais
    local initial_prices=$(grep "✓.*R\$" "$log_file" | head -13 | sed 's/.*R\$ \([0-9.]*\).*/\1/')
    
    # Extrair preços finais (últimas atualizações)
    local final_prices=$(grep "PRICE UPDATER.*→.*R\$" "$log_file" | tail -13 | sed 's/.*→ R\$ \([0-9.]*\).*/\1/')
    
    # Extrair nomes das ações
    local action_names=$(grep "✓.*R\$" "$log_file" | head -13 | sed 's/✓ \([A-Z0-9]*\).*/\1/')
    
    # Criar arquivo de análise
    cat > "$output_file" << EOF
=== ANÁLISE DE PREÇOS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

AÇÃO,PREÇO_INICIAL,PREÇO_FINAL,VARIAÇÃO_PERCENTUAL,STATUS_ECONÔMICO
EOF
    
    # Processar cada ação
    local i=1
    while IFS= read -r name && IFS= read -r initial && IFS= read -r final; do
        if [[ -n "$name" && -n "$initial" && -n "$final" ]]; then
            # Calcular variação percentual
            local variation=$(echo "scale=2; (($final - $initial) / $initial) * 100" | bc -l 2>/dev/null || echo "0")
            
            # Determinar status econômico
            local status="NORMAL"
            if (( $(echo "$variation > 10" | bc -l) )); then
                status="ALTA_SIGNIFICATIVA"
            elif (( $(echo "$variation < -10" | bc -l) )); then
                status="BAIXA_SIGNIFICATIVA"
            elif (( $(echo "$variation > 5" | bc -l) )); then
                status="ALTA_MODERADA"
            elif (( $(echo "$variation < -5" | bc -l) )); then
                status="BAIXA_MODERADA"
            fi
            
            echo "$name,$initial,$final,$variation,$status" >> "$output_file"
        fi
        i=$((i + 1))
    done < <(echo "$action_names" && echo "$initial_prices" && echo "$final_prices")
    
    log "✓ Dados de preços extraídos para $output_file"
}

# Função para analisar impacto das ordens nos preços
analyze_order_impact() {
    local log_file=$1
    local output_file=$2
    
    log_info "Analisando impacto das ordens nos preços"
    
    cat > "$output_file" << EOF
=== ANÁLISE DE IMPACTO DAS ORDENS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

TIMESTAMP,TRADER_ID,AÇÃO,TIPO_ORDEM,PREÇO_ORDEM,QUANTIDADE,STATUS,IMPACTO_PREÇO
EOF
    
    # Extrair ordens e seus impactos
    grep -E "(NOVA ORDEM|EXECUTADA|CANCELADA)" "$log_file" | while read -r line; do
        local timestamp=$(echo "$line" | grep -o '\[[^]]*\]' | head -1 | sed 's/[][]//g')
        local trader_id=$(echo "$line" | grep -o 'Trader [0-9]*' | grep -o '[0-9]*')
        local action_name=$(echo "$line" | grep -o '[A-Z0-9]*' | head -1)
        local order_type=$(echo "$line" | grep -o 'compra\|vende' | head -1)
        local price=$(echo "$line" | grep -o 'R\$ [0-9.]*' | grep -o '[0-9.]*')
        local quantity=$(echo "$line" | grep -o '[0-9]* ações' | grep -o '[0-9]*')
        local status=$(echo "$line" | grep -o 'EXECUTADA\|CANCELADA' | head -1 || echo "PENDENTE")
        
        # Determinar impacto no preço
        local impact="BAIXO"
        if [[ -n "$quantity" && "$quantity" -gt 500 ]]; then
            impact="ALTO"
        elif [[ -n "$quantity" && "$quantity" -gt 200 ]]; then
            impact="MÉDIO"
        fi
        
        if [[ -n "$timestamp" && -n "$trader_id" ]]; then
            echo "$timestamp,$trader_id,$action_name,$order_type,$price,$quantity,$status,$impact" >> "$output_file"
        fi
    done
    
    log "✓ Análise de impacto das ordens concluída"
}

# Função para detectar race conditions
detect_race_conditions() {
    local log_file=$1
    local output_file=$2
    
    log_info "Detectando race conditions"
    
    cat > "$output_file" << EOF
=== DETECÇÃO DE RACE CONDITIONS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

TIMESTAMP,TIPO_RACE,DESCRIÇÃO,SEVERIDADE,IMPACTO
EOF
    
    # Detectar padrões de race conditions
    local race_count=0
    
    # 1. Verificar atualizações simultâneas de preços
    local simultaneous_updates=$(grep "PRICE UPDATER" "$log_file" | awk '{print $1}' | sort | uniq -d | wc -l)
    if [[ $simultaneous_updates -gt 0 ]]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S'),PREÇO_SIMULTÂNEO,Atualizações simultâneas de preços detectadas,ALTA,Preços inconsistentes" >> "$output_file"
        race_count=$((race_count + 1))
    fi
    
    # 2. Verificar ordens processadas fora de ordem
    local out_of_order=$(grep "EXECUTOR: Processando" "$log_file" | awk '{print $2}' | sort -n | awk 'NR>1 && $1<=prev {print "OUT_OF_ORDER"} {prev=$1}' | wc -l)
    if [[ $out_of_order -gt 0 ]]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S'),ORDEM_FORA_SEQUÊNCIA,Ordens processadas fora de sequência,MÉDIA,Execução inconsistente" >> "$output_file"
        race_count=$((race_count + 1))
    fi
    
    # 3. Verificar preços irrealistas
    local unrealistic_prices=$(grep "PRICE UPDATER.*→.*R\$" "$log_file" | awk -F'→' '{print $2}' | grep -o 'R\$ [0-9.]*' | sed 's/R\$ //' | awk '$1 < 0.1 || $1 > 1000 {print "UNREALISTIC"}' | wc -l)
    if [[ $unrealistic_prices -gt 0 ]]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S'),PREÇO_IRREALISTA,Preços fora do range realista,ALTA,Distorção de mercado" >> "$output_file"
        race_count=$((race_count + 1))
    fi
    
    # 4. Verificar variações extremas
    local extreme_variations=$(grep "PRICE UPDATER.*→.*R\$" "$log_file" | grep -o '([^)]*)' | sed 's/[()%]//g' | awk '$1 > 50 || $1 < -50 {print "EXTREME"}' | wc -l)
    if [[ $extreme_variations -gt 0 ]]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S'),VARIAÇÃO_EXTREMA,Variações de preço extremas (>50%),ALTA,Volatilidade anômala" >> "$output_file"
        race_count=$((race_count + 1))
    fi
    
    log "✓ Detecção de race conditions concluída ($race_count encontradas)"
}

# Função para calcular eficiência do mercado
calculate_market_efficiency() {
    local log_file=$1
    local output_file=$2
    
    log_info "Calculando eficiência do mercado"
    
    # Extrair métricas de execução
    local total_orders=$(grep -c "NOVA ORDEM" "$log_file")
    local executed_orders=$(grep -c "EXECUTADA" "$log_file")
    local cancelled_orders=$(grep -c "CANCELADA\|REJEITOU" "$log_file")
    
    # Calcular tempos de execução
    local execution_times=$(grep "EXECUTOR.*ACEITOU.*em.*ms" "$log_file" | grep -o 'em [0-9]*ms' | sed 's/em //' | sed 's/ms//')
    local avg_execution_time=0
    local min_execution_time=999999
    local max_execution_time=0
    local count=0
    
    while IFS= read -r time; do
        if [[ -n "$time" && "$time" -gt 0 ]]; then
            avg_execution_time=$(echo "$avg_execution_time + $time" | bc -l)
            if [[ $time -lt $min_execution_time ]]; then
                min_execution_time=$time
            fi
            if [[ $time -gt $max_execution_time ]]; then
                max_execution_time=$time
            fi
            count=$((count + 1))
        fi
    done < <(echo "$execution_times")
    
    if [[ $count -gt 0 ]]; then
        avg_execution_time=$(echo "scale=2; $avg_execution_time / $count" | bc -l)
    fi
    
    # Calcular latência média
    local latency_avg=0
    if [[ $count -gt 0 ]]; then
        latency_avg=$avg_execution_time
    fi
    
    # Calcular throughput
    local total_time=$(grep "PRICE UPDATER" "$log_file" | tail -1 | awk '{print $1}' | sed 's/\[//' | sed 's/\]//')
    local start_time=$(grep "PRICE UPDATER" "$log_file" | head -1 | awk '{print $1}' | sed 's/\[//' | sed 's/\]//')
    
    # Converter para segundos (aproximado)
    local duration_seconds=30  # Valor padrão se não conseguir calcular
    
    if [[ -n "$total_time" && -n "$start_time" ]]; then
        # Tentar calcular duração real
        local start_epoch=$(date -d "$start_time" +%s 2>/dev/null || echo "0")
        local end_epoch=$(date -d "$total_time" +%s 2>/dev/null || echo "0")
        if [[ $start_epoch -gt 0 && $end_epoch -gt 0 ]]; then
            duration_seconds=$((end_epoch - start_epoch))
        fi
    fi
    
    local throughput=$(echo "scale=2; $executed_orders / $duration_seconds" | bc -l 2>/dev/null || echo "0")
    
    # Calcular eficiência
    local efficiency=$(echo "scale=2; ($executed_orders / $total_orders) * 100" | bc -l 2>/dev/null || echo "0")
    
    cat > "$output_file" << EOF
=== EFICIÊNCIA DO MERCADO - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

MÉTRICA,VALOR,UNIDADE,DESCRIÇÃO
Total de Ordens,$total_orders,ordens,Total de ordens criadas
Ordens Executadas,$executed_orders,ordens,Ordens executadas com sucesso
Ordens Canceladas,$cancelled_orders,ordens,Ordens canceladas ou rejeitadas
Taxa de Execução,$efficiency,%,Percentual de ordens executadas
Tempo Médio de Execução,$avg_execution_time,ms,Tempo médio para executar uma ordem
Tempo Mínimo de Execução,$min_execution_time,ms,Tempo mínimo de execução
Tempo Máximo de Execução,$max_execution_time,ms,Tempo máximo de execução
Throughput,$throughput,ordens/segundo,Ordens processadas por segundo
Latência Média,$latency_avg,ms,Latência média do sistema
Duração do Teste,$duration_seconds,segundos,Duração total do teste
Eficiência Geral,$(echo "scale=2; ($efficiency * $throughput) / 100" | bc -l),índice,Índice de eficiência geral
EOF
    
    log "✓ Cálculo de eficiência do mercado concluído"
}

# Função para gerar relatório consolidado
generate_consolidated_report() {
    local report_file="$ANALYSIS_DIR/relatorio_consolidado_$(date +%Y%m%d_%H%M%S).txt"
    
    log_info "Gerando relatório consolidado"
    
    cat > "$report_file" << EOF
=== RELATÓRIO CONSOLIDADO DE ANÁLISE DE PREÇOS E EFICIÊNCIA ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

## RESUMO EXECUTIVO

Este relatório apresenta uma análise completa dos preços finais, impacto das ordens,
race conditions detectadas e eficiência do mercado no sistema de trading.

## 1. ANÁLISE DE PREÇOS FINAIS

### 1.1 Preços Iniciais vs Finais
$(cat "$ANALYSIS_DIR"/*_precos.csv 2>/dev/null | grep -v "===" | tail -n +3 | head -10 || echo "Dados não disponíveis")

### 1.2 Status Econômico das Ações
- NORMAL: Variação entre -5% e +5%
- ALTA_MODERADA: Variação entre +5% e +10%
- BAIXA_MODERADA: Variação entre -10% e -5%
- ALTA_SIGNIFICATIVA: Variação > +10%
- BAIXA_SIGNIFICATIVA: Variação < -10%

## 2. IMPACTO DAS ORDENS NOS PREÇOS

### 2.1 Distribuição por Tipo de Ordem
$(cat "$ANALYSIS_DIR"/*_ordens.csv 2>/dev/null | grep -v "===" | tail -n +3 | awk -F',' '{print $4}' | sort | uniq -c || echo "Dados não disponíveis")

### 2.2 Impacto por Volume
- BAIXO: < 200 ações
- MÉDIO: 200-500 ações
- ALTO: > 500 ações

## 3. RACE CONDITIONS DETECTADAS

### 3.1 Tipos de Race Conditions
$(cat "$ANALYSIS_DIR"/*_race_conditions.csv 2>/dev/null | grep -v "===" | tail -n +3 | awk -F',' '{print $2}' | sort | uniq -c || echo "Nenhuma race condition detectada")

### 3.2 Severidade das Race Conditions
- BAIXA: Impacto mínimo no sistema
- MÉDIA: Pode causar inconsistências menores
- ALTA: Pode causar distorções significativas

## 4. EFICIÊNCIA DO MERCADO

### 4.1 Métricas Principais
$(cat "$ANALYSIS_DIR"/*_eficiencia.csv 2>/dev/null | grep -v "===" | tail -n +3 | head -5 || echo "Dados não disponíveis")

### 4.2 Análise de Performance
- Taxa de Execução: Indica eficiência do sistema
- Throughput: Capacidade de processamento
- Latência: Tempo de resposta do sistema

## 5. RECOMENDAÇÕES

### 5.1 Para Melhorar Preços
- Implementar mecanismos de correção de preços irrealistas
- Adicionar validações de range de preços
- Implementar circuit breakers para variações extremas

### 5.2 Para Reduzir Race Conditions
- Melhorar sincronização entre threads
- Implementar locks mais granulares
- Adicionar verificações de consistência

### 5.3 Para Aumentar Eficiência
- Otimizar algoritmos de execução
- Reduzir latência de processamento
- Implementar cache de dados

## 6. CONCLUSÕES

O sistema demonstra:
- Variações de preço realistas na maioria dos casos
- Algumas race conditions que precisam ser corrigidas
- Eficiência moderada que pode ser melhorada
- Necessidade de monitoramento contínuo

EOF
    
    log "✓ Relatório consolidado gerado: $report_file"
}

# Função principal
main() {
    log "=== INICIANDO ANÁLISE DE PREÇOS E EFICIÊNCIA DO MERCADO ==="
    
    create_directories
    
    # Processar cada arquivo de log
    for log_file in "$LOG_DIR"/*.log; do
        if [[ -f "$log_file" ]]; then
            local base_name=$(basename "$log_file" .log)
            
            log "Processando $base_name"
            
            # Extrair dados de preços
            extract_price_data "$log_file" "$ANALYSIS_DIR/${base_name}_precos.csv"
            
            # Analisar impacto das ordens
            analyze_order_impact "$log_file" "$ANALYSIS_DIR/${base_name}_ordens.csv"
            
            # Detectar race conditions
            detect_race_conditions "$log_file" "$ANALYSIS_DIR/${base_name}_race_conditions.csv"
            
            # Calcular eficiência do mercado
            calculate_market_efficiency "$log_file" "$ANALYSIS_DIR/${base_name}_eficiencia.csv"
        fi
    done
    
    # Gerar relatório consolidado
    generate_consolidated_report
    
    log "=== ANÁLISE CONCLUÍDA ==="
    log "Arquivos gerados em: $ANALYSIS_DIR"
    log "Relatório consolidado disponível"
}

# Executar função principal
main 