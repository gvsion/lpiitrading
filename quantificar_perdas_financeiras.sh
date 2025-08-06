#!/bin/bash

# Script para Quantificar Perdas Financeiras por Race Conditions
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
ANALYSIS_DIR="$SCRIPT_DIR/analysis"
RESULTS_DIR="$SCRIPT_DIR/results"

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

# Função para calcular perdas por corrupção de preços
calculate_price_corruption_losses() {
    local log_file=$1
    local output_file=$2
    
    log_info "Calculando perdas por corrupção de preços"
    
    # Extrair dados de preços
    local price_changes=$(grep "PRICE UPDATER.*→.*R\$" "$log_file" | grep -o '([^)]*)' | sed 's/[()%]//g')
    local volumes=$(grep "EXECUTADA.*comprou\|EXECUTADA.*vendeu" "$log_file" | grep -o '[0-9]* ações' | grep -o '[0-9]*')
    local prices=$(grep "EXECUTADA.*a R\$" "$log_file" | grep -o 'R\$ [0-9.]*' | sed 's/R\$ //')
    
    local total_loss=0
    local count=0
    
    # Calcular perdas por variação de preço
    while IFS= read -r change && IFS= read -r volume && IFS= read -r price; do
        if [[ -n "$change" && -n "$volume" && -n "$price" ]]; then
            # Converter para números
            change=$(echo "$change" | sed 's/[^0-9.-]//g')
            volume=$(echo "$volume" | sed 's/[^0-9]//g')
            price=$(echo "$price" | sed 's/[^0-9.]//g')
            
            if [[ $change != 0 && $volume -gt 0 && $price -gt 0 ]]; then
                # Calcular perda
                local loss=$(echo "scale=2; $volume * $price * $change / 100" | bc -l 2>/dev/null || echo "0")
                total_loss=$(echo "$total_loss + $loss" | bc -l 2>/dev/null || echo "$total_loss")
                count=$((count + 1))
            fi
        fi
    done < <(echo "$price_changes" && echo "$volumes" && echo "$prices")
    
    # Calcular médias
    local avg_loss=0
    if [[ $count -gt 0 ]]; then
        avg_loss=$(echo "scale=2; $total_loss / $count" | bc -l 2>/dev/null || echo "0")
    fi
    
    # Salvar resultados
    cat > "$output_file" << EOF
=== PERDAS POR CORRUPÇÃO DE PREÇOS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

MÉTRICA,VALOR,UNIDADE,DESCRIÇÃO
Total de Variações,$count,variações,Número de variações de preço analisadas
Perda Total,R$ $total_loss,reais,Perda total por corrupção de preços
Perda Média,R$ $avg_loss,reais,Perda média por variação
Perda por Hora,R$ $(echo "scale=2; $total_loss * 3600 / 30" | bc -l 2>/dev/null || echo "0"),reais/hora,Projeção por hora
Perda por Dia,R$ $(echo "scale=2; $total_loss * 3600 * 8 / 30" | bc -l 2>/dev/null || echo "0"),reais/dia,Projeção por dia (8h)
Perda por Mês,R$ $(echo "scale=2; $total_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0"),reais/mês,Projeção por mês (22 dias)
EOF
    
    log "✓ Perdas por corrupção de preços calculadas: R$ $total_loss"
}

# Função para calcular perdas por ordens perdidas
calculate_lost_orders_losses() {
    local log_file=$1
    local output_file=$2
    
    log_info "Calculando perdas por ordens perdidas"
    
    # Extrair ordens rejeitadas
    local rejected_orders=$(grep "EXECUTOR.*REJEITOU\|EXECUTOR.*CANCELADA" "$log_file")
    local total_rejected=0
    local total_value_lost=0
    
    while IFS= read -r line; do
        if [[ -n "$line" ]]; then
            # Extrair quantidade e preço
            local quantity=$(echo "$line" | grep -o '[0-9]* ações' | head -1 | grep -o '[0-9]*')
            local price=$(echo "$line" | grep -o 'R\$ [0-9.]*' | head -1 | sed 's/R\$ //')
            
            if [[ -n "$quantity" && -n "$price" ]]; then
                # Calcular valor perdido
                local value_lost=$(echo "scale=2; $quantity * $price" | bc -l 2>/dev/null || echo "0")
                total_value_lost=$(echo "$total_value_lost + $value_lost" | bc -l 2>/dev/null || echo "$total_value_lost")
                total_rejected=$((total_rejected + 1))
            fi
        fi
    done < <(echo "$rejected_orders")
    
    # Calcular médias
    local avg_value_lost=0
    if [[ $total_rejected -gt 0 ]]; then
        avg_value_lost=$(echo "scale=2; $total_value_lost / $total_rejected" | bc -l 2>/dev/null || echo "0")
    fi
    
    # Salvar resultados
    cat > "$output_file" << EOF
=== PERDAS POR ORDENS PERDIDAS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

MÉTRICA,VALOR,UNIDADE,DESCRIÇÃO
Total de Ordens Rejeitadas,$total_rejected,ordens,Número de ordens rejeitadas
Valor Total Perdido,R$ $total_value_lost,reais,Valor total das ordens perdidas
Valor Médio por Ordem,R$ $avg_value_lost,reais,Valor médio por ordem rejeitada
Perda por Hora,R$ $(echo "scale=2; $total_value_lost * 3600 / 30" | bc -l 2>/dev/null || echo "0"),reais/hora,Projeção por hora
Perda por Dia,R$ $(echo "scale=2; $total_value_lost * 3600 * 8 / 30" | bc -l 2>/dev/null || echo "0"),reais/dia,Projeção por dia (8h)
Perda por Mês,R$ $(echo "scale=2; $total_value_lost * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0"),reais/mês,Projeção por mês (22 dias)
EOF
    
    log "✓ Perdas por ordens perdidas calculadas: R$ $total_value_lost"
}

# Função para calcular perdas por corrupção de arrays
calculate_array_corruption_losses() {
    local log_file=$1
    local output_file=$2
    
    log_info "Calculando perdas por corrupção de arrays"
    
    # Contar race conditions em arrays
    local array_races=$(grep -c "WRITE_ARRAY.*ORDEM" "$log_file" 2>/dev/null || echo "0")
    local total_corrupted=0
    local total_value_corrupted=0
    
    # Estimar perdas baseadas em race conditions
    if [[ $array_races -gt 0 ]]; then
        # Assumir que cada race condition corrompe 3 ordens
        total_corrupted=$((array_races * 3))
        
        # Estimar valor médio por ordem corrompida
        local avg_order_value=3072.72  # Baseado nos dados observados
        total_value_corrupted=$(echo "scale=2; $total_corrupted * $avg_order_value" | bc -l 2>/dev/null || echo "0")
    fi
    
    # Salvar resultados
    cat > "$output_file" << EOF
=== PERDAS POR CORRUPÇÃO DE ARRAYS - $(basename "$log_file") ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

MÉTRICA,VALOR,UNIDADE,DESCRIÇÃO
Race Conditions em Arrays,$array_races,condições,Número de race conditions em arrays
Ordens Corrompidas,$total_corrupted,ordens,Ordens estimadas corrompidas
Valor Total Corrompido,R$ $total_value_corrupted,reais,Valor total das ordens corrompidas
Valor Médio por Ordem,R$ 3072.72,reais,Valor médio por ordem (estimado)
Perda por Hora,R$ $(echo "scale=2; $total_value_corrupted * 3600 / 30" | bc -l 2>/dev/null || echo "0"),reais/hora,Projeção por hora
Perda por Dia,R$ $(echo "scale=2; $total_value_corrupted * 3600 * 8 / 30" | bc -l 2>/dev/null || echo "0"),reais/dia,Projeção por dia (8h)
Perda por Mês,R$ $(echo "scale=2; $total_value_corrupted * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0"),reais/mês,Projeção por mês (22 dias)
EOF
    
    log "✓ Perdas por corrupção de arrays calculadas: R$ $total_value_corrupted"
}

# Função para gerar relatório consolidado
generate_consolidated_loss_report() {
    local report_file="$RESULTS_DIR/relatorio_perdas_financeiras_$(date +%Y%m%d_%H%M%S).txt"
    
    log_info "Gerando relatório consolidado de perdas financeiras"
    
    # Calcular totais
    local total_price_loss=0
    local total_order_loss=0
    local total_array_loss=0
    
    # Somar perdas de todos os arquivos
    for file in "$ANALYSIS_DIR"/*_price_losses.csv; do
        if [[ -f "$file" ]]; then
            local loss=$(grep "Perda Total" "$file" | cut -d',' -f2 | sed 's/R$ //' | sed 's/[^0-9.]//g')
            total_price_loss=$(echo "$total_price_loss + $loss" | bc -l 2>/dev/null || echo "$total_price_loss")
        fi
    done
    
    for file in "$ANALYSIS_DIR"/*_order_losses.csv; do
        if [[ -f "$file" ]]; then
            local loss=$(grep "Valor Total Perdido" "$file" | cut -d',' -f2 | sed 's/R$ //' | sed 's/[^0-9.]//g')
            total_order_loss=$(echo "$total_order_loss + $loss" | bc -l 2>/dev/null || echo "$total_order_loss")
        fi
    done
    
    for file in "$ANALYSIS_DIR"/*_array_losses.csv; do
        if [[ -f "$file" ]]; then
            local loss=$(grep "Valor Total Corrompido" "$file" | cut -d',' -f2 | sed 's/R$ //' | sed 's/[^0-9.]//g')
            total_array_loss=$(echo "$total_array_loss + $loss" | bc -l 2>/dev/null || echo "$total_array_loss")
        fi
    done
    
    # Calcular total geral
    local total_loss=$(echo "$total_price_loss + $total_order_loss + $total_array_loss" | bc -l 2>/dev/null || echo "0")
    
    cat > "$report_file" << EOF
=== RELATÓRIO CONSOLIDADO DE PERDAS FINANCEIRAS ===
Data: $(date '+%Y-%m-%d %H:%M:%S')

## RESUMO EXECUTIVO

Este relatório quantifica as perdas financeiras hipotéticas causadas por race conditions
no sistema de trading, baseado nos dados coletados durante os testes.

## 1. PERDAS POR TIPO DE RACE CONDITION

### 1.1 Corrupção de Preços
- Perda Total: R$ $total_price_loss
- Projeção Mensal: R$ $(echo "scale=2; $total_price_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0")

### 1.2 Ordens Perdidas
- Perda Total: R$ $total_order_loss
- Projeção Mensal: R$ $(echo "scale=2; $total_order_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0")

### 1.3 Corrupção de Arrays
- Perda Total: R$ $total_array_loss
- Projeção Mensal: R$ $(echo "scale=2; $total_array_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0")

## 2. IMPACTO TOTAL

### 2.1 Perdas Totais
- Perda Total: R$ $total_loss
- Projeção por Hora: R$ $(echo "scale=2; $total_loss * 3600 / 30" | bc -l 2>/dev/null || echo "0")
- Projeção por Dia: R$ $(echo "scale=2; $total_loss * 3600 * 8 / 30" | bc -l 2>/dev/null || echo "0")
- Projeção por Mês: R$ $(echo "scale=2; $total_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0")

### 2.2 Análise de Impacto
- **Alto Impacto**: Perdas podem chegar a R$ $(echo "scale=2; $total_loss * 3600 * 8 * 22 / 30" | bc -l 2>/dev/null || echo "0") por mês
- **Crítico**: Race conditions podem comprometer a viabilidade do sistema
- **Urgente**: Correções devem ser implementadas imediatamente

## 3. RECOMENDAÇÕES

### 3.1 Ações Imediatas
1. Implementar correções para race conditions identificadas
2. Adicionar verificações de consistência
3. Implementar monitoramento contínuo
4. Treinar equipe em concorrência

### 3.2 Ações de Médio Prazo
1. Revisar arquitetura do sistema
2. Implementar testes de stress
3. Estabelecer métricas de qualidade
4. Documentar boas práticas

### 3.3 Ações de Longo Prazo
1. Implementar sistema de auditoria
2. Desenvolver ferramentas de detecção
3. Estabelecer processos de revisão
4. Criar cultura de qualidade

## 4. CONCLUSÕES

As race conditions identificadas no sistema de trading podem causar perdas
financeiras significativas, comprometendo a viabilidade do sistema em produção.

É essencial implementar correções imediatas e estabelecer processos de
monitoramento contínuo para garantir a integridade e confiabilidade do sistema.

---

**Data da Análise**: $(date '+%Y-%m-%d %H:%M:%S')
**Arquivos Analisados**: $(ls "$LOG_DIR"/*.log | wc -l) logs
**Race Conditions Detectadas**: $(grep -r "race condition\|RACE" "$ANALYSIS_DIR" | wc -l) total
**Perdas Financeiras Estimadas**: R$ $total_loss total
EOF
    
    log "✓ Relatório consolidado gerado: $report_file"
}

# Função principal
main() {
    log "=== INICIANDO QUANTIFICAÇÃO DE PERDAS FINANCEIRAS ==="
    
    # Criar diretórios
    mkdir -p "$RESULTS_DIR"
    
    # Processar cada arquivo de log
    for log_file in "$LOG_DIR"/*.log; do
        if [[ -f "$log_file" ]]; then
            local base_name=$(basename "$log_file" .log)
            
            log "Processando $base_name"
            
            # Calcular perdas por tipo
            calculate_price_corruption_losses "$log_file" "$ANALYSIS_DIR/${base_name}_price_losses.csv"
            calculate_lost_orders_losses "$log_file" "$ANALYSIS_DIR/${base_name}_order_losses.csv"
            calculate_array_corruption_losses "$log_file" "$ANALYSIS_DIR/${base_name}_array_losses.csv"
        fi
    done
    
    # Gerar relatório consolidado
    generate_consolidated_loss_report
    
    log "=== QUANTIFICAÇÃO CONCLUÍDA ==="
    log "Arquivos gerados em: $ANALYSIS_DIR"
    log "Relatório consolidado em: $RESULTS_DIR"
}

# Executar função principal
main 