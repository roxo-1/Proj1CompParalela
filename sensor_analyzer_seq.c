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

    SensorStats stats[MAX_SENSORES];
    memset(stats, 0, sizeof(stats));

    char line[150];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Lendo e processando...\n");

    while (fgets(line, sizeof(line), file)) {
        char *token;

        char id[15], tipo[15], status[10];
        double valor;

        // Parsing rápido
        token = strtok(line, " ");
        if (!token) continue;
        strcpy(id, token);

        strtok(NULL, " ");
        strtok(NULL, " ");

        token = strtok(NULL, " ");
        if (!token) continue;
        strcpy(tipo, token);

        token = strtok(NULL, " ");
        if (!token) continue;
        valor = atof(token);

        strtok(NULL, " "); // "status"

        token = strtok(NULL, " \n");
        if (!token) continue;
        strcpy(status, token);

        // Converter ID -> índice
        int idx = atoi(&id[7]) - 1;
        if (idx < 0 || idx >= MAX_SENSORES) continue;

        SensorStats *s = &stats[idx];

        strcpy(s->id, id);
        s->ativo = 1;

        // Processamento otimizado
        if (tipo[0] == 't') { // temperatura
            s->soma_temp += valor;
            s->soma_quadrados_temp += valor * valor;
            s->count_temp++;
        } else if (tipo[0] == 'e') { // energia
            s->energia_total += valor;
        }

        // Alertas
        if (status[0] == 'A' || status[0] == 'C') {
            s->total_alertas++;
        }
    }

    fclose(file);

    // Análise final
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
            desvio = sqrt((stats[i].soma_quadrados_temp / stats[i].count_temp) - (media * media));

            if (exibidos < 10) {
                printf("Sensor: %s | Média: %.2f°C | Desvio: %.2f\n",
                       stats[i].id, media, desvio);
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

    // Cálculo de tempo CORRETO (robusto)
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    double tempo_total = seconds + nanoseconds / 1e9;

    printf("\n--- ESTATÍSTICAS GERAIS ---\n");
    printf("Sensor mais instável: %s (Desvio Padrão: %.2f)\n",
           sensor_instavel, maior_desvio);
    printf("Total de alertas (ALERTA/CRITICO): %d\n", alertas_globais);
    printf("Consumo total de energia: %.2f W\n", consumo_total_energia);
    printf("Tempo de execução: %.4f segundos\n", tempo_total);

    return 0;
}
