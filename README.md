# Sistema de Trading - Bolsa de Valores

Este é um sistema completo de trading simulado em C, implementado com duas versões: uma usando **threads** e outra usando **processos**. O sistema simula um ambiente de negociação de ações com múltiplos traders, atualização de preços em tempo real, execução de ordens e monitoramento de oportunidades de arbitragem.

## 🏗️ Arquitetura do Sistema

### Estrutura de Arquivos

```
.
├── trading_system.h          # Header principal com estruturas e constantes
├── trader.c                  # Módulo de traders e estratégias
├── executor.c                # Módulo de execução de ordens
├── price_updater.c           # Módulo de atualização de preços
├── arbitrage_monitor.c       # Módulo de monitoramento de arbitragem
├── main_threads.c            # Versão usando threads
├── main_processos.c          # Versão usando processos
├── Makefile                  # Script de compilação
└── README.md                 # Este arquivo
```

### Componentes do Sistema

1. **Traders (6 traders)**: Cada trader possui uma estratégia diferente
   - Trader Conservador
   - Trader Agressivo
   - Trader Momentum
   - Trader Mean Reversion
   - Trader Arbitragem
   - Trader Aleatório

2. **Ações (10 ações)**: PETR4, VALE3, ITUB4, BBDC4, ABEV3, WEGE3, RENT3, LREN3, MGLU3, JBSS3

3. **Executor**: Responsável por processar e executar ordens

4. **Price Updater**: Atualiza preços das ações em tempo real

5. **Arbitrage Monitor**: Monitora oportunidades de arbitragem

## 🚀 Como Usar

### Pré-requisitos

```bash
# Instalar dependências
sudo apt update
sudo apt install build-essential gdb valgrind
```

### Compilação

```bash
# Compilar ambas as versões
make all

# Ou compilar individualmente
make trading_threads    # Versão threads
make trading_processos  # Versão processos
```

### Execução

```bash
# Executar versão threads
make run-threads

# Executar versão processos
make run-processos

# Executar ambas as versões
make run
```

### Debug

```bash
# Debug com valgrind
make debug-threads
make debug-processos
```

### Outros Comandos

```bash
make clean        # Limpar arquivos compilados
make deps         # Verificar dependências
make help         # Mostrar ajuda completa
```

## 📊 Estruturas de Dados

### Constantes
- `MAX_ACOES = 10`
- `MAX_TRADERS = 6`
- `MAX_ORDENS = 100`
- `MAX_NOME = 50`

### Estruturas Principais

```c
typedef struct {
    char nome[MAX_NOME];
    double preco_atual;
    double preco_anterior;
    double variacao;
    int volume_negociado;
    pthread_mutex_t mutex;
} Acao;

typedef struct {
    int id;
    char nome[MAX_NOME];
    double saldo;
    int acoes_possuidas[MAX_ACOES];
    pthread_mutex_t mutex;
} Trader;

typedef struct {
    int id;
    int trader_id;
    int acao_id;
    char tipo; // 'C' para compra, 'V' para venda
    double preco;
    int quantidade;
    time_t timestamp;
    int status; // 0: pendente, 1: executada, 2: cancelada
} Ordem;
```

## 🎯 Estratégias de Trading

### 1. Trader Conservador
- Compra quando preço cai mais de 5%
- Vende quando preço sobe mais de 5%
- Opera com volumes menores

### 2. Trader Agressivo
- Opera com volumes maiores
- Segue tendências de mercado
- Maior frequência de operações

### 3. Trader Momentum
- Segue a tendência do mercado
- Compra em alta, vende em baixa
- Baseado em variações de preço

### 4. Trader Mean Reversion
- Acredita que preços voltam à média
- Compra quando preço está muito baixo
- Vende quando preço está muito alto

### 5. Trader Arbitragem
- Procura diferenças de preço entre ações
- Identifica oportunidades de arbitragem
- Opera quando há discrepâncias

### 6. Trader Aleatório
- Toma decisões baseadas em probabilidade
- 30% de chance de comprar
- 30% de chance de vender

## 🔄 Versões do Sistema

### Versão Threads (`main_threads.c`)
- Usa threads POSIX (`pthread`)
- Memória compartilhada entre threads
- Sincronização com mutexes e semáforos
- Mais eficiente para comunicação
- Menor overhead de criação

### Versão Processos (`main_processos.c`)
- Usa processos (`fork()`)
- Memória compartilhada via `shmget()`
- Comunicação via memória compartilhada
- Maior isolamento entre componentes
- Melhor para sistemas distribuídos

## 📈 Funcionalidades

### Atualização de Preços
- Simulação realista de variações
- Histórico de preços
- Cálculo de volatilidade
- Simulação de notícias de mercado

### Execução de Ordens
- Validação de saldo e ações
- Execução automática
- Cancelamento de ordens inválidas
- Estatísticas de execução

### Monitoramento de Arbitragem
- Detecção de oportunidades
- Alertas de mercado
- Análise de padrões
- Estatísticas de arbitragem

### Estatísticas em Tempo Real
- Estado das ações
- Estado dos traders
- Estatísticas do executor
- Oportunidades de arbitragem
- Alertas de mercado

## 🛠️ Desenvolvimento

### Adicionando Novas Estratégias

1. Adicione a estratégia no enum `EstrategiaTrader`
2. Implemente a função da estratégia
3. Adicione o case no switch em `executar_estrategia_trader()`

### Adicionando Novas Ações

1. Adicione o nome e preço inicial em `inicializar_acoes()`
2. Atualize a constante `MAX_ACOES` se necessário

### Modificando Parâmetros

- Frequência de operações: `dados_traders[i].frequencia_operacao`
- Saldo inicial: `sistema->traders[i].saldo = 100000.0`
- Volatilidade: `historicos[i].volatilidade = 0.02`

## 🔍 Debug e Monitoramento

### Logs do Sistema
- Eventos importantes são logados
- Timestamps em todas as operações
- Rastreamento de ordens

### Valgrind
- Detecção de memory leaks
- Análise de uso de memória
- Verificação de erros

### Estatísticas
- Taxa de execução de ordens
- Volume de negociação
- Oportunidades de arbitragem
- Performance dos traders

## 🚨 Troubleshooting

### Problemas Comuns

1. **Erro de compilação**
   ```bash
   make clean
   make all
   ```

2. **Erro de permissão**
   ```bash
   chmod +x trading_threads trading_processos
   ```

3. **Erro de memória compartilhada**
   ```bash
   # Limpar segmentos de memória compartilhada
   ipcs -m | awk '{print $2}' | xargs -I {} ipcrm -m {}
   ```

4. **Processos órfãos**
   ```bash
   # Encontrar e matar processos
   ps aux | grep trading
   kill -9 <PID>
   ```

## 📝 Licença

Este projeto é para fins educacionais e de demonstração. Não deve ser usado para trading real.

## 🤝 Contribuições

Contribuições são bem-vindas! Por favor:

1. Fork o projeto
2. Crie uma branch para sua feature
3. Commit suas mudanças
4. Push para a branch
5. Abra um Pull Request

## 📞 Suporte

Para dúvidas ou problemas:

1. Verifique a documentação
2. Execute `make help`
3. Teste com `make test-compile`
4. Verifique as dependências com `make deps` 