#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST     1
#define FAULT    2
#define RECOVERY 3

typedef struct {
    int id; // identificador de facility do SMPL
            // outras variáveis locais de cada processo são declaradas aqui
} Processo;

Processo *processos;

int main(int argc, char **argv) {
    int MaxTempoSimulac = 120;
    
    char fa_name[5]; // facility name
    
    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <número de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // número de processos do sistema distribuído

    smpl(0, "Meu primeiro programa de simulação de sistemas distribuídos");
    reset();
    stream(1);

    // inicializar os N processos
    processos = malloc(sizeof(Processo) * N);
    for (int i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%c", (char)i);
        processos[i].id = facility(fa_name, 1);
    }

    // escalonamento dos eventos iniciais
    for (int i = 0; i < N; i++) {
        schedule(TEST, 30.0, i); // todos vão testar na unidade de tempo 30
    }
    schedule(FAULT, 31.0, 1);
    schedule(RECOVERY, 61.0, 1);

    // loop principal do simulador
    int token; // o processo com o token é o que está executando agora
    int event;
    while (time() < MaxTempoSimulac) {
        cause(&event, &token);
        switch (event) {
            case TEST:
                if (status(processos[token].id) != 0) {
                    break; // processo falho não testa
                }
                printf("Sou o processo %d e estou testando no tempo %4.1f\n", token, time());
                schedule(TEST, 30.0, token);
                break;
            case FAULT:
                request(processos[token].id, token, 0);
                printf("O processo %d falhou no tempo %4.1f\n", token, time());
                break;
            case RECOVERY:
                release(processos[token].id, token);
                printf("O processo %d recuperou no tempo %4.1f\n", token, time());
                schedule(TEST, 1.0, token);
                break;
            default:
                break;
        }
    }

    return 0;
}
