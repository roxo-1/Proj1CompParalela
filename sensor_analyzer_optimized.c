#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_SENSORES 1000
#define MAX_LINHAS 1000000

typedef struct {
    char id[15];
    char tipo[15];
    double valor;
    char status[10];
} LogEntry;

typedef struct {
    char id[15];
    double soma_temp;
    double soma_quadrados_temp;
    int count_temp;
    double energia_total;
    int total_alertas;
    int ativo;
} SensorStats;

typedef struct {
    int inicio;
    int fim;
    LogEntry *logs;
    SensorStats local_stats[MAX_SENSORES];
} ThreadArgs;

// Extrai índice do sensor com segurança
int get_sensor_index(const char *id) {
    if (strlen(id) < 8) return -1;
    return atoi(id + 7) - 1;
}

// Função da thread
void* processar_bloco(void* arg) {
    ThreadArgs *data = (ThreadArgs*)arg;

    for (int i = data->inicio; i < data->fim; i++) {
        int idx = get_sensor_index(data->logs[i].id);
        if (idx < 0 || idx >= MAX_SENSORES) continue;

        SensorStats *s = &data->local_stats[idx];

        s->ativo = 1;
        strcpy(s->id, data->logs[i].id);

        if (strcmp(data->logs[i].tipo, "temperatura") == 0) {
            double v = data->logs[i].valor;
            s->soma_temp += v;
            s->soma_quadrados_temp += v * v;
            s->count_temp++;
        } else if (strcmp(data->logs[i].tipo, "energia") == 0) {
            s->energia_total += data->logs[i].valor;
        }

        if (strcmp(data->logs[i].status, "ALERTA") == 0 ||
            strcmp(data->logs[i].status, "CRITICO") == 0) {
            s->total_alertas++;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Uso: %s <threads> <arquivo.log>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        printf("Número de threads inválido!\n");
        return 1;
    }

    char *nome_arquivo = argv[2];

    FILE *file = fopen(nome_arquivo, "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    LogEntry *logs = malloc(sizeof(LogEntry) * MAX_LINHAS);
    if (!logs) {
        printf("Erro de memória!\n");
        fclose(file);
        return 1;
    }

    char line[150];
    int total_lido = 0;

    printf("Carregando arquivo...\n");

    while (fgets(line, sizeof(line), file) && total_lido < MAX_LINHAS) {
        if (sscanf(line, "%14s %*s %*s %14s %lf status %9s",
                   logs[total_lido].id,
                   logs[total_lido].tipo,
                   &logs[total_lido].valor,
                   logs[total_lido].status) == 4) {
            total_lido++;
        }
    }

    fclose(file);

    printf("Total de linhas: %d\n", total_lido);

    clock_t start = clock();

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    ThreadArgs *t_args = malloc(sizeof(ThreadArgs) * num_threads);

    if (!threads || !t_args) {
        printf("Erro de memória!\n");
        free(logs);
        return 1;
    }

    int chunk = total_lido / num_threads;

    // Inicializa stats locais
    for (int i = 0; i < num_threads; i++) {
        memset(t_args[i].local_stats, 0, sizeof(t_args[i].local_stats));
    }

    // Criação das threads
    for (int i = 0; i < num_threads; i++) {
        t_args[i].inicio = i * chunk;
        t_args[i].fim = (i == num_threads - 1) ? total_lido : (i + 1) * chunk;
        t_args[i].logs = logs;

        pthread_create(&threads[i], NULL, processar_bloco, &t_args[i]);
    }

    // Espera threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Merge final
    SensorStats final_stats[MAX_SENSORES];
    memset(final_stats, 0, sizeof(final_stats));

    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < MAX_SENSORES; i++) {
            if (!t_args[t].local_stats[i].ativo) continue;

            final_stats[i].ativo = 1;
            strcpy(final_stats[i].id, t_args[t].local_stats[i].id);

            final_stats[i].soma_temp += t_args[t].local_stats[i].soma_temp;
            final_stats[i].soma_quadrados_temp += t_args[t].local_stats[i].soma_quadrados_temp;
            final_stats[i].count_temp += t_args[t].local_stats[i].count_temp;
            final_stats[i].energia_total += t_args[t].local_stats[i].energia_total;
            final_stats[i].total_alertas += t_args[t].local_stats[i].total_alertas;
        }
    }

    // Resultados
    double maior_desvio = -1;
    char sensor_instavel[15] = "";
    double consumo_total = 0;
    int alertas = 0;

    printf("\n--- RESULTADOS ---\n");

    for (int i = 0; i < MAX_SENSORES; i++) {
        if (!final_stats[i].ativo) continue;

        if (final_stats[i].count_temp > 0) {
            double media = final_stats[i].soma_temp / final_stats[i].count_temp;
            double desvio = sqrt((final_stats[i].soma_quadrados_temp / final_stats[i].count_temp) - (media * media));

            printf("Sensor: %s | Media: %.2f | Desvio: %.2f\n",
                   final_stats[i].id, media, desvio);

            if (desvio > maior_desvio) {
                maior_desvio = desvio;
                strcpy(sensor_instavel, final_stats[i].id);
            }
        }

        consumo_total += final_stats[i].energia_total;
        alertas += final_stats[i].total_alertas;
    }

    clock_t end = clock();
    double tempo = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\nSensor mais instavel: %s\n", sensor_instavel);
    printf("Total alertas: %d\n", alertas);
    printf("Consumo total: %.2f\n", consumo_total);
    printf("Tempo: %.4f segundos\n", tempo);

    free(logs);
    free(threads);
    free(t_args);

    return 0;
}