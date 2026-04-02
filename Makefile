# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -O2 -pthread

# Executáveis
TARGETS = sensor_analyzer_seq sensor_analyzer_par sensor_analyzer_optimized

# Arquivos fonte
SRC_SEQ = sensor_analyzer_seq.c
SRC_PAR = sensor_analyzer_par.c
SRC_OPT = sensor_analyzer_optimized.c

# Regra padrão
all: $(TARGETS)

# Versão sequencial
sensor_analyzer_seq: $(SRC_SEQ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Versão paralela com mutex
sensor_analyzer_par: $(SRC_PAR)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Versão otimizada
sensor_analyzer_optimized: $(SRC_OPT)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Limpeza
clean:
	rm -f $(TARGETS)

# Recompilar tudo
rebuild: clean all