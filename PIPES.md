# Sistema de Pipes - Comunicação entre Processos

## 📋 Resumo do Sistema

Este documento descreve a implementação do sistema de comunicação entre processos usando pipes anônimos. O sistema permite a comunicação bidirecional entre diferentes componentes do sistema de trading através de uma arquitetura bem definida.

## 🏗️ Arquitetura de Comunicação

### Fluxo de Comunicação:
```
Traders → Executor → Price Updater → Arbitrage Monitor
    ↑                                    ↓
    └────────── Feedback ────────────────┘
```

### Pipes Implementados:

1. **Traders → Executor**: Envio de ordens de compra/venda
2. **Executor → Price Updater**: Atualizações de preços após execução
3. **Price Updater → Arbitrage Monitor**: Detecção de oportunidades de arbitragem
4. **Arbitrage Monitor → Traders**: Feedback sobre oportunidades detectadas
5. **Controle**: Comunicação de controle geral do sistema

## 🎯 Funcionalidades Implementadas

### 1. **Criação de Pipes do Sistema**

#### `criar_pipes_sistema()` (TAREFA DO ALUNO)
- **Descrição**: Cria todos os pipes necessários para comunicação entre processos
- **Retorno**: Array de descritores de arquivo (10 descritores para 5 pipes)
- **Funcionalidades**:
  - Cria 5 pipes anônimos
  - Configura pipes como não-bloqueantes
  - Retorna array de descritores
  - Tratamento de erros para falhas na criação

**Exemplo de uso:**
```c
int* descritores = criar_pipes_sistema();
if (!descritores) {
    printf("Erro: Falha ao criar pipes do sistema\n");
    return 1;
}
```

### 2. **Tratamento de Erros para Falhas na Criação de Pipes** (TAREFA DO ALUNO)

**Implementado em `criar_pipes_sistema()`:**
- Verificação de retorno de `pipe()`
- Limpeza automática em caso de falha
- Mensagens de erro detalhadas
- Rollback de pipes já criados

**Exemplo de tratamento:**
```c
if (pipe(sistema_pipes.traders_to_executor) == -1) {
    perror("ERRO: Falha ao criar pipe Traders -> Executor");
    return NULL;
}
```

### 3. **Gerenciamento de Descritores de Arquivo**

#### `limpar_pipes_sistema()`
- **Descrição**: Fecha todos os descritores de arquivo dos pipes
- **Funcionalidades**:
  - Fecha descritores de leitura e escrita
  - Verifica se descritores são válidos antes de fechar
  - Log de fechamento de cada pipe
  - Reset do status do sistema

#### `pipes_estao_ativos()`
- **Descrição**: Verifica se os pipes estão ativos
- **Retorno**: 1 se ativos, 0 caso contrário

#### `imprimir_status_pipes()`
- **Descrição**: Exibe status detalhado dos pipes
- **Informações exibidas**:
  - Número de pipes criados
  - Status de ativação
  - Descritores de cada pipe

### 4. **Comunicação via Mensagens**

#### Estrutura de Mensagem:
```c
typedef struct {
    int tipo_mensagem;     // 1: ordem, 2: atualização, 3: arbitragem, 4: controle
    int origem_id;         // ID do processo origem
    int destino_id;        // ID do processo destino
    int dados_ordem;       // Dados da ordem (simplificado)
    double valor;          // Valor da operação
    char dados_extras[100]; // Dados adicionais
    time_t timestamp;      // Timestamp da mensagem
} MensagemPipe;
```

#### Funções de Envio/Recebimento:
- `enviar_mensagem_pipe()`: Envia mensagem via pipe
- `receber_mensagem_pipe()`: Recebe mensagem via pipe
- Tratamento de erros para pipes cheios/vazios
- Modo não-bloqueante

### 5. **Criação de Mensagens Específicas**

#### `criar_mensagem_ordem()`
- **Descrição**: Cria mensagem de ordem de compra/venda
- **Parâmetros**: trader_id, acao_id, tipo, preco, quantidade

#### `criar_mensagem_atualizacao_preco()`
- **Descrição**: Cria mensagem de atualização de preço
- **Parâmetros**: acao_id, preco_anterior, preco_novo

#### `criar_mensagem_arbitragem()`
- **Descrição**: Cria mensagem de detecção de arbitragem
- **Parâmetros**: acao1_id, acao2_id, diferenca, percentual

#### `criar_mensagem_controle()`
- **Descrição**: Cria mensagem de controle do sistema
- **Parâmetros**: comando, origem_id, destino_id

### 6. **Integração com Processos**

#### Atualização de `main_processos.c`:
- Criação de pipes antes do `fork()`
- Fechamento de descritores desnecessários em cada processo filho
- Integração com memória compartilhada
- Limpeza de pipes ao parar processos

**Exemplo de uso em processos:**
```c
// Processo pai cria pipes
int* descritores = criar_pipes_sistema();

// Processo filho fecha descritores desnecessários
close(descritores[0]); // Traders->Executor RD
close(descritores[1]); // Traders->Executor WR
// ... outros descritores

// Envio de mensagem
MensagemPipe ordem = criar_mensagem_ordem(0, 1, 'C', 25.50, 100);
enviar_mensagem_pipe(descritores[1], &ordem);
```

## 🧪 Testes Implementados

### 1. **Teste de Criação de Pipes**
- Verificação de criação bem-sucedida
- Exibição de descritores
- Status dos pipes

### 2. **Teste de Envio e Recebimento**
- Envio de mensagens de teste
- Recebimento e verificação
- Diferentes tipos de mensagem

### 3. **Teste de Comunicação Simulada**
- Simulação do fluxo completo
- Traders → Executor → Price Updater → Arbitrage Monitor
- Feedback para traders

### 4. **Teste de Tratamento de Erros**
- Envio para pipe inválido
- Recebimento de pipe vazio
- Verificação de comportamentos esperados

### 5. **Teste de Múltiplas Mensagens**
- Envio de múltiplas mensagens
- Recebimento em sequência
- Verificação de integridade

### 6. **Teste de Criação e Fechamento** (TAREFA DO ALUNO)
- Segunda criação de pipes
- Fechamento correto
- Verificação de status após fechamento

### 7. **Teste Automático**
- Função `testar_pipes_sistema()`
- Teste completo de todas as funcionalidades
- Limpeza automática

## ✅ Tarefas do Aluno Implementadas

### 1. **Implementação de `criar_pipes_sistema()`**

**Funcionalidades completas:**
- Criação de 5 pipes anônimos
- Array de retorno com 10 descritores
- Configuração não-bloqueante
- Log detalhado de criação
- Tratamento de erros robusto

**Código implementado:**
```c
int* criar_pipes_sistema() {
    printf("=== CRIANDO PIPES DO SISTEMA ===\n");
    
    // Inicializar estrutura
    memset(&sistema_pipes, 0, sizeof(SistemaPipes));
    
    // Array para retornar descritores
    static int descritores[10]; // 5 pipes * 2 descritores cada
    int indice = 0;
    
    // Criar cada pipe com tratamento de erro
    if (pipe(sistema_pipes.traders_to_executor) == -1) {
        perror("ERRO: Falha ao criar pipe Traders -> Executor");
        return NULL;
    }
    // ... continua para todos os pipes
    
    return descritores;
}
```

### 2. **Adição de Tratamento de Erro para Falhas na Criação de Pipes**

**Implementado:**
- Verificação de retorno de `pipe()`
- Limpeza automática em caso de falha
- Mensagens de erro detalhadas
- Rollback de pipes já criados

**Exemplo de tratamento:**
```c
if (pipe(sistema_pipes.executor_to_price_updater) == -1) {
    perror("ERRO: Falha ao criar pipe Executor -> Price Updater");
    limpar_pipes_sistema(); // Limpa pipes já criados
    return NULL;
}
```

### 3. **Teste de Criação e Fechamento dos Pipes**

**Implementado em `test_pipes.c`:**
- Segunda criação de pipes
- Verificação de sucesso
- Fechamento correto
- Verificação de status após fechamento

**Código de teste:**
```c
printf("Testando criação de pipes...\n");
int* novos_descriptores = criar_pipes_sistema();
if (novos_descriptores) {
    printf("✓ Segunda criação de pipes bem-sucedida\n");
    imprimir_status_pipes();
    
    printf("Testando fechamento de pipes...\n");
    limpar_pipes_sistema();
    
    if (!pipes_estao_ativos()) {
        printf("✓ Pipes fechados corretamente\n");
    }
}
```

## 📊 Estruturas de Dados

### Estrutura `SistemaPipes`:
```c
typedef struct {
    int traders_to_executor[2];      // Traders -> Executor
    int executor_to_price_updater[2]; // Executor -> Price Updater
    int price_updater_to_arbitrage[2]; // Price Updater -> Arbitrage Monitor
    int arbitrage_to_traders[2];     // Arbitrage Monitor -> Traders (feedback)
    int control_pipe[2];             // Controle geral do sistema
    int num_pipes_criados;
    int pipes_ativos;
} SistemaPipes;
```

### Estrutura `MensagemPipe`:
```c
typedef struct {
    int tipo_mensagem;     // 1: ordem, 2: atualização, 3: arbitragem, 4: controle
    int origem_id;         // ID do processo origem
    int destino_id;        // ID do processo destino
    int dados_ordem;       // Dados da ordem (simplificado)
    double valor;          // Valor da operação
    char dados_extras[100]; // Dados adicionais
    time_t timestamp;      // Timestamp da mensagem
} MensagemPipe;
```

## 🔧 Como Usar

### Compilação:
```bash
make test-compile
```

### Execução dos Testes:
```bash
make run-test-pipes
```

### Execução Manual:
```bash
./test_pipes
```

## 📈 Funcionalidades Avançadas

### 1. **Comunicação Bidirecional**
- Fluxo principal: Traders → Executor → Price Updater → Arbitrage Monitor
- Feedback: Arbitrage Monitor → Traders
- Controle: Comunicação geral do sistema

### 2. **Modo Não-Bloqueante**
- Pipes configurados como não-bloqueantes
- Tratamento de pipes cheios/vazios
- Comunicação assíncrona

### 3. **Gerenciamento de Descritores**
- Fechamento automático de descritores desnecessários
- Limpeza adequada ao finalizar
- Prevenção de vazamentos de descritores

### 4. **Tratamento de Erros Robusto**
- Verificação de criação de pipes
- Rollback em caso de falha
- Mensagens de erro detalhadas

### 5. **Integração com Processos**
- Criação antes do `fork()`
- Fechamento em processos filhos
- Limpeza ao parar processos

## 🎯 Resultados dos Testes

### ✅ **Testes Bem-sucedidos:**
- Criação de pipes do sistema
- Verificação de status dos pipes
- Envio e recebimento de mensagens
- Comunicação entre processos simulada
- Tratamento de erros
- Múltiplas mensagens
- Criação e fechamento de pipes
- Teste automático dos pipes
- Gerenciamento correto de descritores de arquivo

### 📊 **Métricas Observadas:**
- **5 pipes** criados com sucesso
- **10 descritores** gerenciados corretamente
- **4 tipos de mensagem** implementados
- **Comunicação bidirecional** funcional
- **Tratamento de erros** robusto
- **Limpeza adequada** de recursos

## 🔍 Melhorias Futuras

### 1. **Funcionalidades Adicionais**
- Pipes nomeados para persistência
- Comunicação multicast
- Priorização de mensagens
- Compressão de dados

### 2. **Monitoramento Avançado**
- Métricas de throughput
- Latência de comunicação
- Detecção de gargalos
- Logs estruturados

### 3. **Segurança**
- Validação de mensagens
- Criptografia de dados
- Controle de acesso
- Auditoria de comunicação

## 🎉 Conclusão

O sistema de pipes foi implementado com sucesso, incluindo:

✅ **Criação robusta** de pipes do sistema  
✅ **Tratamento de erros** para falhas na criação  
✅ **Teste completo** de criação e fechamento  
✅ **Gerenciamento correto** de descritores de arquivo  
✅ **Comunicação bidirecional** entre processos  
✅ **Integração perfeita** com o sistema de trading  
✅ **Modo não-bloqueante** para comunicação assíncrona  
✅ **Limpeza adequada** de recursos  
✅ **Testes abrangentes** com casos reais  

O sistema demonstra uma arquitetura de comunicação robusta e escalável para sistemas distribuídos. 