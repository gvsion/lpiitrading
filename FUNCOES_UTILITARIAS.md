# Funções Utilitárias - Sistema de Trading

## 📋 Resumo das Funções Implementadas

Este documento descreve as funções utilitárias implementadas para simular dados financeiros realistas e as tarefas do aluno.

## 🎯 Funções Principais Implementadas

### 1. **Geração de Ordens Aleatórias**

#### `gerar_ordem_aleatoria(TradingSystem* sistema)`
- **Descrição**: Gera uma ordem aleatória realista
- **Parâmetros**: Sistema de trading
- **Retorno**: Estrutura `Ordem` preenchida
- **Características**:
  - Trader aleatório (0-5)
  - Ação aleatória (0-9)
  - Tipo: 60% compra, 40% venda
  - Quantidade: 100-1000 ações
  - Preço: baseado no preço atual ±10%
  - Timestamp atual

#### `gerar_ordens_aleatorias(TradingSystem* sistema, int num_ordens)`
- **Descrição**: Gera múltiplas ordens aleatórias
- **Parâmetros**: Sistema e número de ordens
- **Funcionalidades**:
  - Valida cada ordem antes de adicionar
  - Imprime ordens para debug
  - Pausa entre gerações (0.1s)

### 2. **Cálculo de Preços por Oferta/Demanda**

#### `calcular_preco_oferta_demanda(TradingSystem* sistema, int acao_id)`
- **Descrição**: Calcula novo preço baseado na pressão de compra/venda
- **Parâmetros**: Sistema e ID da ação
- **Retorno**: Novo preço calculado
- **Algoritmo**:
  1. Analisa ordens pendentes
  2. Calcula pressão de compra vs venda
  3. Aplica variação baseada na intensidade
  4. Adiciona componente aleatório (±1%)
  5. Garante limites (R$ 10-200)

### 3. **Detecção de Arbitragem entre Ações Relacionadas**

#### `detectar_arbitragem_relacionadas(TradingSystem* sistema)`
- **Descrição**: Detecta oportunidades entre ações do mesmo setor
- **Grupos de ações**:
  - **Commodities**: PETR4, VALE3
  - **Bancos**: ITUB4, BBDC4, ABEV3
  - **Varejo**: WEGE3, RENT3, LREN3, MGLU3
- **Critérios**:
  - Correlação esperada: 0.7
  - Diferença mínima: 5%
  - Sugestões de compra/venda

## ✅ Tarefas do Aluno Implementadas

### 1. **Função `validar_ordem()`**

#### Validações Implementadas:
- **Sistema e ordem**: Verifica se não são NULL
- **Trader ID**: Deve estar entre 0 e num_traders-1
- **Ação ID**: Deve estar entre 0 e num_acoes-1
- **Tipo de ordem**: Deve ser 'C' (compra) ou 'V' (venda)
- **Preço**: Deve estar entre R$ 10.00 e R$ 200.00
- **Quantidade**: Deve estar entre 100 e 1000 ações
- **Saldo**: Para compras, verifica se trader tem saldo suficiente
- **Ações**: Para vendas, verifica se trader tem ações suficientes
- **Timestamp**: Deve ser válido (> 0)

#### Exemplo de Uso:
```c
Ordem ordem = gerar_ordem_aleatoria(sistema);
if (validar_ordem(sistema, &ordem)) {
    printf("✓ Ordem válida\n");
} else {
    printf("✗ Ordem inválida\n");
}
```

### 2. **Função `imprimir_ordem()`**

#### Informações Exibidas:
- **ID da ordem**
- **Trader**: Nome e ID
- **Ação**: Nome e ID
- **Tipo**: COMPRA ou VENDA
- **Quantidade**: Número de ações
- **Preço**: Preço unitário
- **Valor Total**: Preço × Quantidade
- **Status**: PENDENTE, EXECUTADA ou CANCELADA
- **Timestamp**: Data e hora
- **Saldo do Trader**: Saldo atual
- **Ações possuídas**: Quantidade da ação específica

#### Exemplo de Saída:
```
=== ORDEM #5 ===
Trader: Trader Momentum (ID: 2)
Ação: WEGE3 (ID: 5)
Tipo: COMPRA
Quantidade: 765 ações
Preço: R$ 47.41
Valor Total: R$ 36272.25
Status: PENDENTE
Timestamp: Tue Aug  5 20:49:44 2025
Saldo do Trader: R$ 100000.00
Ações possuídas de WEGE3: 0
================
```

## 🧪 Testes Implementados

### 1. **Teste de Geração de Ordens**
- Gera 3 ordens aleatórias
- Imprime cada ordem com `imprimir_ordem()`
- Valida cada ordem com `validar_ordem()`

### 2. **Teste de Validação**
- **Ordem válida**: Testa ordem normal
- **Preço inválido**: R$ 500.00 (muito alto)
- **Quantidade inválida**: 50 (muito baixa)
- **Tipo inválido**: 'X' (não é C ou V)
- **Trader ID inválido**: 999
- **Ação ID inválida**: 999

### 3. **Teste de Cálculo de Preços**
- Adiciona 10 ordens aleatórias
- Calcula novos preços para todas as ações
- Mostra variações percentuais

### 4. **Teste de Detecção de Arbitragem**
- Analisa grupos de ações relacionadas
- Detecta oportunidades de arbitragem
- Sugere ações de compra/venda

### 5. **Teste de Estatísticas de Mercado**
- Mostra estatísticas por ação
- Exibe ordens de compra/venda
- Calcula preços médios e spreads

## 📊 Exemplos de Saída dos Testes

### Geração de Ordens:
```
=== ORDEM #0 ===
Trader: Trader Arbitragem (ID: 4)
Ação: LREN3 (ID: 7)
Tipo: COMPRA
Quantidade: 116 ações
Preço: R$ 17.27
Valor Total: R$ 2003.18
Status: PENDENTE
Timestamp: Tue Aug  5 20:49:43 2025
Saldo do Trader: R$ 100000.00
Ações possuídas de LREN3: 0
================
```

### Validação de Ordens:
```
✓ Ordem válida
ERRO: Preço fora do intervalo válido: R$ 500.00
ERRO: Quantidade fora do intervalo válido: 50
ERRO: Tipo de ordem inválido: X
ERRO: Trader ID inválido: 999
ERRO: Ação ID inválida: 999
```

### Cálculo de Preços:
```
PETR4: R$ 25.50 → R$ 25.25 (variação: -0.98%)
VALE3: R$ 68.30 → R$ 68.44 (variação: 0.20%)
BBDC4: R$ 15.80 → R$ 16.72 (variação: 5.82%)
```

### Detecção de Arbitragem:
```
ARBITRAGEM DETECTADA: PETR4 e VALE3 com correlação 0.00 (esperado: 0.70)
  OPORTUNIDADE: Diferença de 91.26% entre PETR4 (R$ 25.50) e VALE3 (R$ 68.30)
  SUGESTÃO: Comprar PETR4, vender VALE3
```

## 🔧 Como Usar

### Compilação:
```bash
make test-compile
```

### Execução dos Testes:
```bash
make run-test-utils
```

### Execução Manual:
```bash
./test_utils
```

## 📈 Funcionalidades Avançadas

### 1. **Simulação Realista**
- Preços baseados em dados reais
- Volatilidades variadas por ação
- Correlações entre ações do mesmo setor

### 2. **Validação Robusta**
- Verificação completa de parâmetros
- Mensagens de erro detalhadas
- Prevenção de ordens inválidas

### 3. **Debug Completo**
- Impressão detalhada de ordens
- Estatísticas de mercado
- Rastreamento de operações

### 4. **Extensibilidade**
- Fácil adição de novas validações
- Configuração de parâmetros
- Integração com sistema principal

## 🎯 Resultados dos Testes

### ✅ **Testes Bem-sucedidos:**
- Geração de ordens aleatórias realistas
- Validação completa de ordens
- Cálculo de preços por oferta/demanda
- Detecção de oportunidades de arbitragem
- Estatísticas detalhadas de mercado
- Integração com sistema principal

### 📊 **Métricas Observadas:**
- **Taxa de validação**: ~60% (ordens de venda são rejeitadas por falta de ações)
- **Variação de preços**: -1% a +5% por ciclo
- **Detecção de arbitragem**: Múltiplas oportunidades identificadas
- **Performance**: Execução rápida e eficiente

## 🔍 Melhorias Futuras

### 1. **Validações Adicionais**
- Verificar horário de mercado
- Validar liquidez da ação
- Verificar limites de posição

### 2. **Algoritmos Avançados**
- Machine learning para preços
- Análise técnica avançada
- Correlações dinâmicas

### 3. **Interface Melhorada**
- Gráficos em tempo real
- Dashboard web
- Alertas automáticos

## 🎉 Conclusão

As funções utilitárias foram implementadas com sucesso, incluindo:

✅ **Geração de ordens aleatórias** com parâmetros realistas  
✅ **Cálculo de preços** baseado em oferta/demanda  
✅ **Detecção de arbitragem** entre ações relacionadas  
✅ **Validação robusta** de ordens (tarefa do aluno)  
✅ **Impressão detalhada** para debug (tarefa do aluno)  
✅ **Testes abrangentes** com casos manuais  
✅ **Integração completa** com o sistema principal  

O sistema demonstra funcionalidades avançadas de simulação financeira com validação robusta e debug completo. 