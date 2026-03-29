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
} ThreadArgs;

//  Estrutura global compartilhada
SensorStats final_stats[MAX_SENSORES];

//  Mutex global
pthread_mutex_t mutex;

// Função da thread COM mutex
void* processar_bloco(void* arg) {
    ThreadArgs *data = (ThreadArgs*)arg;

    for (int i = data->inicio; i < data->fim; i++) {
        int idx = atoi(&data->logs[i].id[7]) - 1;
        if (idx < 0 || idx >= MAX_SENSORES) continue;

        //  trava antes de acessar dado compartilhado
        pthread_mutex_lock(&mutex);

        SensorStats *s = &final_stats[idx];

        s->ativo = 1;
        strcpy(s->id, data->logs[i].id);

        if (strcmp(data->logs[i].tipo, "temperatura") == 0) {
            s->soma_temp += data->logs[i].valor;
            s->soma_quadrados_temp += data->logs[i].valor * data->logs[i].valor;
            s->count_temp++;
        } else if (strcmp(data->logs[i].tipo, "energia") == 0) {
            s->energia_total += data->logs[i].valor;
        }

        if (strcmp(data->logs[i].status, "ALERTA") == 0 ||
            strcmp(data->logs[i].status, "CRITICO") == 0) {
            s->total_alertas++;
        }

        //  libera o mutex
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Uso: %s <threads> <arquivo.log>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    char *nome_arquivo = argv[2];

    FILE *file = fopen(nome_arquivo, "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    LogEntry *logs = malloc(sizeof(LogEntry) * MAX_LINHAS);
    if (!logs) {
        printf("Erro de memória!\n");
        return 1;
    }

    char line[150];
    int total_lido = 0;

    printf("Carregando arquivo...\n");

    while (fgets(line, sizeof(line), file) && total_lido < MAX_LINHAS) {
        sscanf(line, "%s %*s %*s %s %lf status %s",
               logs[total_lido].id,
               logs[total_lido].tipo,
               &logs[total_lido].valor,
               logs[total_lido].status);
        total_lido++;
    }

    fclose(file);

    printf("Total de linhas: %d\n", total_lido);

    clock_t start = clock();

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    ThreadArgs *t_args = malloc(sizeof(ThreadArgs) * num_threads);

    int chunk = total_lido / num_threads;

    //  inicializa estrutura global
    memset(final_stats, 0, sizeof(final_stats));

    //  inicializa mutex
    pthread_mutex_init(&mutex, NULL);

    // Criação das threads
    for (int i = 0; i < num_threads; i++) {
        t_args[i].inicio = i * chunk;
        t_args[i].fim = (i == num_threads - 1) ? total_lido : (i + 1) * chunk;
        t_args[i].logs = logs;

        pthread_create(&threads[i], NULL, processar_bloco, &t_args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // RESULTADOS
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

    //  destrói mutex
    pthread_mutex_destroy(&mutex);

    free(logs);
    free(threads);
    free(t_args);

    return 0;
}
