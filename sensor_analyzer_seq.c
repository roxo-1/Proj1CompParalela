/*Integrantes:
Carolina Lee 10440304
Enrique Cipolla 10427824
Pedro Gabriel Guimarães Fernandes 10437465*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_SENSORES 1000
#define MAX_LINHAS 10000000 

// Estrutura para armazenar cada linha do log em memória 
typedef struct {
    char id[15];
    char tipo[15];
    double valor;
    char status[10];
} LogEntry;

// Estrutura para estatísticas de cada sensor 
typedef struct {
    char id[15];
    double soma_temp;
    double soma_quadrados_temp;
    int count_temp;
    double energia_total;
    int total_alertas;
    int ativo;
} SensorStats;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo.log>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    LogEntry *logs = malloc(sizeof(LogEntry) * MAX_LINHAS);
    if (!logs) {
        printf("Erro de memória!\n");
        return 1;
    }

    SensorStats stats[MAX_SENSORES];
    memset(stats, 0, sizeof(stats));

    char line[150];
    int total_lido = 0;
    
    // Início da medição de tempo 
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Lendo arquivo e processando dados...\n");

    while (fgets(line, sizeof(line), file) && total_lido < MAX_LINHAS) {
        sscanf(line, "%s %*s %*s %s %lf status %s", 
               logs[total_lido].id, logs[total_lido].tipo, 
               &logs[total_lido].valor, logs[total_lido].status);
        
        int idx = atoi(&logs[total_lido].id[7]) - 1;
        if (idx < 0 || idx >= MAX_SENSORES) continue;

        strcpy(stats[idx].id, logs[total_lido].id);
        stats[idx].ativo = 1;

        // Processamento por tipo de sensor 
        if (strcmp(logs[total_lido].tipo, "temperatura") == 0) {
            stats[idx].soma_temp += logs[total_lido].valor;
            stats[idx].soma_quadrados_temp += (logs[total_lido].valor * logs[total_lido].valor);
            stats[idx].count_temp++;
        } else if (strcmp(logs[total_lido].tipo, "energia") == 0) {
            stats[idx].energia_total += logs[total_lido].valor;
        }

        // Contagem de alertas 
        if (strcmp(logs[total_lido].status, "ALERTA") == 0 || 
            strcmp(logs[total_lido].status, "CRITICO") == 0) {
            stats[idx].total_alertas++;
        }

        total_lido++;
    }

    // Análise Final 
    double maior_desvio = -1.0;
    char sensor_instavel[15] = "";
    double consumo_total_energia = 0;
    int alertas_globais = 0;

    printf("\n--- RESULTADOS (TOP 10 SENSORES DE TEMPERATURA) ---\n");
    int exibidos = 0;
    for (int i = 0; i < MAX_SENSORES; i++) {
        if (!stats[i].ativo) continue;

        double media = 0, desvio = 0;
        if (stats[i].count_temp > 0) {
            media = stats[i].soma_temp / stats[i].count_temp;
            // Fórmula incremental do desvio padrão 
            desvio = sqrt((stats[i].soma_quadrados_temp / stats[i].count_temp) - (media * media));

            if (exibidos < 10) {
                printf("Sensor: %s | Média: %.2f°C | Desvio: %.2f\n", stats[i].id, media, desvio);
                exibidos++;
            }

            if (desvio > maior_desvio) {
                maior_desvio = desvio;
                strcpy(sensor_instavel, stats[i].id); 
            }
        }
        consumo_total_energia += stats[i].energia_total; 
        alertas_globais += stats[i].total_alertas; 
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double tempo_total = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n--- ESTATÍSTICAS GERAIS ---\n");
    printf("Sensor mais instável: %s (Desvio Padrão: %.2f)\n", sensor_instavel, maior_desvio);
    printf("Total de alertas (ALERTA/CRITICO): %d\n", alertas_globais);
    printf("Consumo total de energia: %.2f W\n", consumo_total_energia);
    printf("Tempo de execução: %.4f segundos\n", tempo_total);

    free(logs);
    fclose(file);
    return 0;
}