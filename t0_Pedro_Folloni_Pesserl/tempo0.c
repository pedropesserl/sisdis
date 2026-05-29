// Autor: Pedro Folloni Pesserl GRR20220072
// Data ultima modificacao: 13/05/2026
// Funcionalidade: Programa simples de simulacao de sistemas distribuidos com smpl

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST     1
#define FAULT    2
#define RECOVERY 3

typedef struct {
    int id; // identificador de facility do SMPL
            // outras variaveis locais de cada processo sao declaradas aqui
} Processo;

Processo *processos;

int main(int argc, char **argv) {
    int MaxTempoSimulac = 120;
    
    char fa_name[5]; // facility name
    
    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <numero de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // numero de processos do sistema distribuido

    smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
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
        schedule(TEST, 30.0, i); // todos vao testar na unidade de tempo 30
    }
    schedule(FAULT, 31.0, 1);
    schedule(RECOVERY, 61.0, 1);

    // loop principal do simulador
    int token; // o processo com o token eh o que esta executando agora
    int event;
    while (time() < MaxTempoSimulac) {
        cause(&event, &token);
        switch (event) {
            case TEST:
                if (status(processos[token].id) != 0) {
                    break; // processo falho nao testa
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
