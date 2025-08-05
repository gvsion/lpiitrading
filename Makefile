# Makefile para Sistema de Trading
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -pthread
LIBS = -lm -lpthread

# Arquivos fonte
SOURCES_THREADS = main_threads.c trader.c executor.c price_updater.c arbitrage_monitor.c utils.c
SOURCES_PROCESSOS = main_processos.c trader.c executor.c price_updater.c arbitrage_monitor.c utils.c
HEADERS = trading_system.h

# Executáveis
TARGET_THREADS = trading_threads
TARGET_PROCESSOS = trading_processos
TARGET_TEST_UTILS = test_utils

# Objetos
OBJECTS_THREADS = $(SOURCES_THREADS:.c=.o)
OBJECTS_PROCESSOS = $(SOURCES_PROCESSOS:.c=.o)

# Regra padrão
all: $(TARGET_THREADS) $(TARGET_PROCESSOS) $(TARGET_TEST_UTILS)

# Compilar versão threads
$(TARGET_THREADS): $(OBJECTS_THREADS)
	$(CC) $(OBJECTS_THREADS) -o $(TARGET_THREADS) $(LIBS)
	@echo "Versão threads compilada com sucesso!"

# Compilar versão processos
$(TARGET_PROCESSOS): $(OBJECTS_PROCESSOS)
	$(CC) $(OBJECTS_PROCESSOS) -o $(TARGET_PROCESSOS) $(LIBS)
	@echo "Versão processos compilada com sucesso!"

# Compilar programa de teste das funções utilitárias
$(TARGET_TEST_UTILS): test_utils.c sistema_common.c trader.c executor.c price_updater.c arbitrage_monitor.c utils.c
	$(CC) $(CFLAGS) test_utils.c sistema_common.c trader.c executor.c price_updater.c arbitrage_monitor.c utils.c -o $(TARGET_TEST_UTILS) $(LIBS)
	@echo "Programa de teste das funções utilitárias compilado com sucesso!"

# Compilar arquivos objeto
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Executar versão threads
run-threads: $(TARGET_THREADS)
	./$(TARGET_THREADS)

# Executar versão processos
run-processos: $(TARGET_PROCESSOS)
	./$(TARGET_PROCESSOS)

# Executar programa de teste das funções utilitárias
run-test-utils: $(TARGET_TEST_UTILS)
	./$(TARGET_TEST_UTILS)

# Executar ambas as versões
run: run-threads run-processos

# Debug com valgrind (versão threads)
debug-threads: $(TARGET_THREADS)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_THREADS)

# Debug com valgrind (versão processos)
debug-processos: $(TARGET_PROCESSOS)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_PROCESSOS)

# Limpar arquivos compilados
clean:
	rm -f $(OBJECTS_THREADS) $(OBJECTS_PROCESSOS) $(TARGET_THREADS) $(TARGET_PROCESSOS) $(TARGET_TEST_UTILS)
	rm -f *.o
	@echo "Arquivos compilados removidos!"

# Verificar dependências
deps:
	@echo "Verificando dependências..."
	@echo "GCC: $(shell which gcc)"
	@echo "Make: $(shell which make)"
	@echo "Valgrind: $(shell which valgrind 2>/dev/null || echo 'Não instalado')"

# Instalar dependências (Ubuntu/Debian)
install-deps:
	@echo "Instalando dependências..."
	sudo apt update
	sudo apt install -y build-essential gdb valgrind

# Testar compilação
test-compile: clean all
	@echo "Teste de compilação concluído!"

# Mostrar ajuda
help:
	@echo "=== SISTEMA DE TRADING - MAKEFILE ==="
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make all              - Compilar ambas as versões"
	@echo "  make trading_threads  - Compilar apenas versão threads"
	@echo "  make trading_processos - Compilar apenas versão processos"
	@echo "  make run-threads      - Executar versão threads"
	@echo "  make run-processos    - Executar versão processos"
	@echo "  make run-test-utils   - Executar teste das funções utilitárias"
	@echo "  make run              - Executar ambas as versões"
	@echo "  make debug-threads    - Debug versão threads com valgrind"
	@echo "  make debug-processos  - Debug versão processos com valgrind"
	@echo "  make clean            - Limpar arquivos compilados"
	@echo "  make deps             - Verificar dependências"
	@echo "  make install-deps     - Instalar dependências"
	@echo "  make test-compile     - Testar compilação"
	@echo "  make help             - Mostrar esta ajuda"
	@echo ""
	@echo "Estrutura do projeto:"
	@echo "  - main_threads.c      - Versão usando threads"
	@echo "  - main_processos.c    - Versão usando processos"
	@echo "  - trader.c            - Módulo de traders"
	@echo "  - executor.c          - Módulo de execução de ordens"
	@echo "  - price_updater.c     - Módulo de atualização de preços"
	@echo "  - arbitrage_monitor.c - Módulo de monitoramento de arbitragem"
	@echo "  - utils.c             - Módulo de funções utilitárias"
	@echo "  - test_utils.c        - Programa de teste das funções utilitárias"
	@echo "  - trading_system.h    - Header com estruturas e funções"

.PHONY: all clean run run-threads run-processos debug-threads debug-processos deps install-deps test-compile help 