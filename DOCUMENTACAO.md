# Documentação Técnica - Sistema de Trading

## 📋 Resumo do Projeto

Este sistema de trading simulado em C implementa um ambiente completo de negociação de ações com duas versões arquiteturais:

1. **Versão Threads** (`main_threads.c`): Usa threads POSIX para concorrência
2. **Versão Processos** (`main_processos.c`): Usa processos separados com memória compartilhada

## 🏗️ Arquitetura do Sistema

### Componentes Principais

#### 1. **TradingSystem** (Estrutura Central)
```c
typedef struct {
    Acao acoes[MAX_ACOES];           // Array de ações
    Trader traders[MAX_TRADERS];      // Array de traders
    Ordem ordens[MAX_ORDENS];         // Array de ordens
    Executor executor;                // Sistema executor
    int num_acoes, num_traders, num_ordens;
    pthread_mutex_t mutex_geral;      // Mutex global
    sem_t sem_ordens;                 // Semáforo para ordens
    int sistema_ativo;                // Flag de controle
} TradingSystem;
```

#### 2. **Ação** (Estrutura de Ação)
```c
typedef struct {
    char nome[MAX_NOME];              // Nome da ação (ex: PETR4)
    double preco_atual;               // Preço atual
    double preco_anterior;            // Preço anterior
    double variacao;                  // Variação percentual
    int volume_negociado;             // Volume negociado
    pthread_mutex_t mutex;            // Mutex para thread safety
} Acao;
```

#### 3. **Trader** (Estrutura de Trader)
```c
typedef struct {
    int id;                           // ID do trader
    char nome[MAX_NOME];              // Nome do trader
    double saldo;                     // Saldo disponível
    int acoes_possuidas[MAX_ACOES];   // Quantidade de cada ação
    pthread_mutex_t mutex;            // Mutex para thread safety
} Trader;
```

#### 4. **Ordem** (Estrutura de Ordem)
```c
typedef struct {
    int id;                           // ID da ordem
    int trader_id;                    // ID do trader
    int acao_id;                      // ID da ação
    char tipo;                        // 'C' para compra, 'V' para venda
    double preco;                     // Preço da ordem
    int quantidade;                   // Quantidade
    time_t timestamp;                 // Timestamp
    int status;                       // 0: pendente, 1: executada, 2: cancelada
} Ordem;
```

## 🎯 Estratégias de Trading Implementadas

### 1. **Trader Conservador**
- **Lógica**: Compra quando preço cai >5%, vende quando sobe >5%
- **Volume**: Operações pequenas (10 ações)
- **Frequência**: Baixa (7 segundos)

### 2. **Trader Agressivo**
- **Lógica**: Segue tendências de mercado
- **Volume**: Operações grandes (50 ações)
- **Frequência**: Alta (5 segundos)

### 3. **Trader Momentum**
- **Lógica**: Compra em alta, vende em baixa
- **Volume**: Médio (20 ações)
- **Frequência**: Média (6 segundos)

### 4. **Trader Mean Reversion**
- **Lógica**: Acredita que preços voltam à média
- **Volume**: Médio (15 ações)
- **Frequência**: Média (8 segundos)

### 5. **Trader Arbitragem**
- **Lógica**: Procura diferenças de preço entre ações
- **Volume**: Variável (10 ações)
- **Frequência**: Alta (5 segundos)

### 6. **Trader Aleatório**
- **Lógica**: Decisões baseadas em probabilidade
- **Volume**: Pequeno (5 ações)
- **Frequência**: Variável (6 segundos)

## 🔄 Comparação entre Versões

### Versão Threads
**Vantagens:**
- Menor overhead de criação
- Comunicação mais eficiente
- Menor uso de memória
- Sincronização mais simples

**Desvantagens:**
- Menor isolamento entre componentes
- Um erro pode afetar todo o sistema
- Debugging mais complexo

### Versão Processos
**Vantagens:**
- Maior isolamento entre componentes
- Melhor para sistemas distribuídos
- Mais robusto a falhas
- Debugging mais fácil

**Desvantagens:**
- Maior overhead de criação
- Comunicação mais complexa
- Maior uso de memória
- Sincronização mais complexa

## 📊 Funcionalidades Implementadas

### 1. **Atualização de Preços**
- Simulação realista de variações
- Histórico de preços (últimos 100)
- Cálculo de volatilidade
- Simulação de notícias de mercado

### 2. **Execução de Ordens**
- Validação de saldo e ações
- Execução automática
- Cancelamento de ordens inválidas
- Estatísticas de execução

### 3. **Monitoramento de Arbitragem**
- Detecção de oportunidades
- Alertas de mercado
- Análise de padrões
- Estatísticas de arbitragem

### 4. **Estatísticas em Tempo Real**
- Estado das ações
- Estado dos traders
- Estatísticas do executor
- Oportunidades de arbitragem

## 🛠️ Sincronização e Concorrência

### Mutexes Utilizados
- `mutex_geral`: Protege acesso global ao sistema
- `acao.mutex`: Protege dados de cada ação
- `trader.mutex`: Protege dados de cada trader
- `executor.mutex`: Protege dados do executor

### Semáforos Utilizados
- `sem_ordens`: Sinaliza novas ordens para processamento

### Memória Compartilhada (Versão Processos)
- `shmget()`: Cria segmento de memória compartilhada
- `shmat()`: Anexa memória compartilhada
- `shmdt()`: Desanexa memória compartilhada
- `shmctl()`: Remove segmento de memória

## 📈 Algoritmos Implementados

### 1. **Geração de Preços**
```c
double variacao_base = (rand() % 200 - 100) / 10000.0; // ±1%
double variacao_volatilidade = (rand() % 200 - 100) / 10000.0 * volatilidade;
double variacao_tendencia = (preco_medio - preco_atual) / preco_atual * 0.1;
double variacao_total = variacao_base + variacao_volatilidade + variacao_tendencia;
```

### 2. **Detecção de Arbitragem**
```c
double diferenca = fabs(acao1->preco_atual - acao2->preco_atual);
double media = (acao1->preco_atual + acao2->preco_atual) / 2.0;
double percentual_diferenca = diferenca / media;
if (percentual_diferenca > 0.02) {
    // Oportunidade de arbitragem detectada
}
```

### 3. **Execução de Ordens**
```c
if (ordem->tipo == 'C') {
    if (trader->saldo >= custo_total) {
        trader->saldo -= custo_total;
        trader->acoes_possuidas[ordem->acao_id] += ordem->quantidade;
        ordem->status = 1; // Executada
    }
}
```

## 🔍 Monitoramento e Debug

### Logs do Sistema
- Timestamps em todas as operações
- Rastreamento de ordens
- Eventos importantes logados

### Valgrind
- Detecção de memory leaks
- Análise de uso de memória
- Verificação de erros

### Estatísticas
- Taxa de execução de ordens
- Volume de negociação
- Oportunidades de arbitragem
- Performance dos traders

## 🚀 Como Executar

### Compilação
```bash
make all                    # Compilar ambas as versões
make trading_threads        # Compilar apenas threads
make trading_processos      # Compilar apenas processos
```

### Execução
```bash
make run-threads           # Executar versão threads
make run-processos         # Executar versão processos
./trading_threads          # Executar diretamente
./trading_processos        # Executar diretamente
```

### Debug
```bash
make debug-threads         # Debug versão threads
make debug-processos       # Debug versão processos
```

## 📝 Constantes Configuráveis

```c
#define MAX_ACOES 10        // Número máximo de ações
#define MAX_TRADERS 6       // Número máximo de traders
#define MAX_ORDENS 100      // Número máximo de ordens
#define MAX_NOME 50         // Tamanho máximo de nomes
```

## 🔧 Extensibilidade

### Adicionando Novas Estratégias
1. Adicione no enum `EstrategiaTrader`
2. Implemente a função da estratégia
3. Adicione o case no switch em `executar_estrategia_trader()`

### Adicionando Novas Ações
1. Adicione nome e preço inicial em `inicializar_acoes()`
2. Atualize `MAX_ACOES` se necessário

### Modificando Parâmetros
- Frequência de operações: `dados_traders[i].frequencia_operacao`
- Saldo inicial: `sistema->traders[i].saldo = 100000.0`
- Volatilidade: `historicos[i].volatilidade = 0.02`

## 🎯 Resultados Esperados

### Comportamento do Sistema
- **Traders**: Operam continuamente com suas estratégias
- **Preços**: Atualizam a cada 3 segundos com variações realistas
- **Ordens**: São executadas automaticamente pelo executor
- **Arbitragem**: Detecta oportunidades de diferenças de preço
- **Estatísticas**: São exibidas em tempo real

### Métricas de Performance
- Taxa de execução de ordens: ~80-90%
- Volume de negociação: Variável
- Oportunidades de arbitragem: Detectadas continuamente
- Performance dos traders: Diferente por estratégia

## 🔒 Considerações de Segurança

- **Validação**: Todas as ordens são validadas antes da execução
- **Sincronização**: Mutexes protegem dados compartilhados
- **Isolamento**: Versão processos oferece maior isolamento
- **Recuperação**: Sistema pode ser reiniciado sem perda de dados

## 📚 Bibliotecas Utilizadas

- `pthread.h`: Threads POSIX
- `sys/ipc.h`, `sys/shm.h`: Memória compartilhada
- `math.h`: Funções matemáticas
- `time.h`: Timestamps
- `signal.h`: Tratamento de sinais
- `semaphore.h`: Semáforos

## 🎉 Conclusão

O sistema implementa com sucesso um ambiente de trading simulado com:

✅ **Duas arquiteturas** (threads e processos)  
✅ **Seis estratégias** de trading diferentes  
✅ **Dez ações** simuladas  
✅ **Execução automática** de ordens  
✅ **Monitoramento** de arbitragem  
✅ **Estatísticas** em tempo real  
✅ **Sincronização** adequada  
✅ **Extensibilidade** para novas funcionalidades  

O sistema demonstra conceitos avançados de programação concorrente em C, incluindo threads, processos, memória compartilhada, sincronização e algoritmos de trading. 