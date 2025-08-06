# Melhorias Implementadas no Processo Trader

## Resumo das Implementações

Este documento descreve as melhorias implementadas no processo trader conforme solicitado nas tarefas do aluno.

## 1. Perfis de Trader

### Implementação de Perfis
- **Perfil Conservador**: Operações menos frequentes, volumes menores, foco em ações estáveis
- **Perfil Agressivo**: Operações frequentes, volumes maiores, maior risco
- **Perfil Day Trader**: Operações muito frequentes, alta agressividade, foco em ações voláteis

### Características dos Perfis

#### Perfil Conservador
```c
- Intervalo: 3-8 segundos entre ordens
- Máximo: 20 ordens por sessão
- Agressividade: 0.3 (baixa)
- Volume médio: 100 ações
- Ações preferidas: PETR4, VALE3
```

#### Perfil Agressivo
```c
- Intervalo: 1-4 segundos entre ordens
- Máximo: 50 ordens por sessão
- Agressividade: 0.8 (alta)
- Volume médio: 500 ações
- Ações preferidas: ITUB4, ABEV3, BBAS3
```

#### Perfil Day Trader
```c
- Intervalo: 1-3 segundos entre ordens
- Máximo: 100 ordens por sessão
- Agressividade: 0.9 (muito alta)
- Volume médio: 200 ações
- Ações preferidas: BBDC4, WEGE3, RENT3, LREN3
```

## 2. Lógica de Parada Inteligente

### Limites Implementados
- **Limite de tempo**: Cada trader para após 300 segundos (5 minutos)
- **Limite de ordens**: Cada perfil tem um limite máximo de ordens por sessão
- **Controle de duração**: Monitoramento do tempo de execução

### Código de Controle
```c
// Verificar limites de tempo e ordens
if (agora - inicio_sessao > perfil->tempo_limite_sessao) {
    printf("Trader %d: Tempo limite atingido (%ds)\n", trader_id, perfil->tempo_limite_sessao);
    break;
}

if (ordens_enviadas >= perfil->max_ordens_por_sessao) {
    printf("Trader %d: Limite de ordens atingido (%d)\n", trader_id, perfil->max_ordens_por_sessao);
    break;
}
```

## 3. Logs Detalhados

### Formato dos Logs
```c
[HH:MM:SS] TRADER X: COMPRA/VENDA Y ações a R$ Z.ZZ (motivo)
```

### Exemplos de Logs
```
[14:30:15] TRADER 0: COMPRA 150 ações a R$ 25.50 (Probabilidade de compra)
[14:30:18] TRADER 1: VENDA 300 ações a R$ 68.30 (Probabilidade de venda)
[14:30:20] TRADER 2: COMPRA 200 ações a R$ 32.15 (Probabilidade de compra)
```

### Informações Registradas
- **Timestamp**: Hora exata da operação
- **Trader ID**: Identificação do trader
- **Tipo de operação**: Compra ou venda
- **Quantidade**: Número de ações
- **Preço**: Preço unitário da ação
- **Motivo**: Justificativa da decisão

## 4. Uso de poll() em vez de select()

### Implementação com poll()
```c
// Configurar poll para timeout
struct pollfd pfd;
pfd.fd = pipes->arbitrage_to_traders[0]; // Pipe de feedback
pfd.events = POLLIN;

// Verificar feedback do arbitrage monitor (usando poll)
int poll_result = poll(&pfd, 1, 100); // Timeout de 100ms
if (poll_result > 0 && (pfd.revents & POLLIN)) {
    MensagemPipe feedback;
    if (receber_mensagem_pipe(pipes->arbitrage_to_traders[0], &feedback) > 0) {
        printf("Trader %d: Recebeu feedback do arbitrage monitor\n", trader_id);
    }
}
```

### Vantagens do poll()
- **Melhor performance**: Lida melhor com descritores acima de 1024
- **Menos overhead**: Não exige manipulação de fd_set
- **Mais eficiente**: Timeout mais preciso
- **Portabilidade**: Funciona melhor em diferentes sistemas

## 5. Comportamento Realista

### Probabilidade de Compra
```c
double calcular_probabilidade_compra(TradingSystem* sistema, int acao_id, PerfilTrader* perfil) {
    Acao* acao = &sistema->acoes[acao_id];
    double variacao = acao->variacao;
    
    // Base: 30% de probabilidade
    double probabilidade = 0.3;
    
    // Se preço caiu, maior probabilidade de compra
    if (variacao < -0.02) { // Caiu mais de 2%
        probabilidade += 0.4;
    } else if (variacao < -0.01) { // Caiu mais de 1%
        probabilidade += 0.2;
    }
    
    // Ajustar pela agressividade do perfil
    probabilidade *= (1.0 + perfil->agressividade);
    
    return probabilidade;
}
```

### Probabilidade de Venda
```c
double calcular_probabilidade_venda(TradingSystem* sistema, int acao_id, PerfilTrader* perfil) {
    Acao* acao = &sistema->acoes[acao_id];
    double variacao = acao->variacao;
    
    // Base: 20% de probabilidade
    double probabilidade = 0.2;
    
    // Se preço subiu, maior probabilidade de venda
    if (variacao > 0.02) { // Subiu mais de 2%
        probabilidade += 0.4;
    } else if (variacao > 0.01) { // Subiu mais de 1%
        probabilidade += 0.2;
    }
    
    // Ajustar pela agressividade do perfil
    probabilidade *= (1.0 + perfil->agressividade);
    
    return probabilidade;
}
```

## 6. Comunicação via Pipes

### Estrutura de Comunicação
```
Traders → Executor → Price Updater → Arbitrage Monitor → Traders
```

### Envio de Ordens
```c
// Enviar ordem via pipe para executor
MensagemPipe msg = criar_mensagem_ordem(trader_id, acao_id, tipo, preco, quantidade);
if (enviar_mensagem_pipe(pipes->traders_to_executor[1], &msg) > 0) {
    printf("Trader %d: Ordem enviada via pipe (total: %d/%d)\n", 
           trader_id, ordens_enviadas, perfil->max_ordens_por_sessao);
}
```

### Recebimento de Feedback
```c
// Verificar feedback do arbitrage monitor
if (poll_result > 0 && (pfd.revents & POLLIN)) {
    MensagemPipe feedback;
    if (receber_mensagem_pipe(pipes->arbitrage_to_traders[0], &feedback) > 0) {
        printf("Trader %d: Recebeu feedback do arbitrage monitor\n", trader_id);
    }
}
```

## 7. Variação entre Ações

### Seleção de Ações
- **Ações preferidas**: Cada perfil tem suas ações preferidas
- **Seleção aleatória**: Escolha aleatória entre as ações preferidas
- **Diversificação**: Diferentes traders focam em diferentes ações

### Código de Seleção
```c
// Escolher ação aleatória das preferidas
int acao_id = perfil->acoes_preferidas[rand() % perfil->num_acoes_preferidas];
Acao* acao = &sistema->acoes[acao_id];
```

## 8. Intervalos Aleatórios

### Geração de Intervalos
```c
int gerar_intervalo_aleatorio(int min, int max) {
    return min + (rand() % (max - min + 1));
}

// Uso no processo
int intervalo = gerar_intervalo_aleatorio(perfil->intervalo_min_ordens, perfil->intervalo_max_ordens);
if (agora - ultima_ordem >= intervalo) {
    // Enviar ordem
}
```

## 9. Arquivos Criados/Modificados

### Novos Arquivos
- `trader_profiles.c`: Implementação dos perfis e funções melhoradas
- `global_vars.c`: Definição das variáveis globais de memória compartilhada

### Arquivos Modificados
- `trading_system.h`: Adicionadas estruturas e constantes para perfis
- `main_processos.c`: Integração com os novos perfis
- `Makefile`: Inclusão dos novos arquivos

## 10. Como Usar

### Compilação
```bash
make clean
make all
```

### Execução
```bash
# Executar versão processos (com perfis melhorados)
make run-processos

# Executar versão threads
make run-threads
```

### Monitoramento
```bash
# Ver logs em tempo real
tail -f /dev/stdout

# Verificar processos
ps aux | grep trading
```

## 11. Estatísticas Finais

Cada trader gera estatísticas detalhadas ao final:
```
=== TRADER X FINALIZADO ===
Duração: XXXs
Ordens enviadas: XX/XX
Perfil: Nome do Perfil
```

## 12. Benefícios das Melhorias

1. **Realismo**: Comportamento mais próximo do mercado real
2. **Diversificação**: Diferentes estratégias por trader
3. **Controle**: Limites de tempo e ordens
4. **Monitoramento**: Logs detalhados para análise
5. **Performance**: Uso de poll() para melhor eficiência
6. **Comunicação**: Sistema de pipes robusto
7. **Flexibilidade**: Fácil modificação de perfis

O sistema agora simula um ambiente de trading muito mais realista e controlado! 