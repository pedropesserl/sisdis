// Autor: Pedro Folloni Pesserl GRR20220072
// Data ultima modificacao: 13/05/2026
// Funcionalidade: Simulacao de sistema distribuido: quando um processo correto testa outro processo correto, obtem informacoes sobre os processos que nao testou nessa rodada

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST     1
#define FAULT    2
#define RECOVERY 3

typedef enum {
    UNKNOWN = -1,
    CORRETO = 0,
    FALHO   = 1,
} State;

typedef struct {
    int id;        // identificador de facility do SMPL
    State *states; // crenca do processo a respeito dos estados dos demais
    bool *tested;  // processos que testou nessa rodada
} Processo;

Processo *processos;

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
    reset();
    stream(1);

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].id = 0;
    }
    for (int i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%c", (char)i);
        processos[i].id = facility(fa_name, 1);
        for (int j = 0; j < N; j++) {
            processos[i].states[j] = UNKNOWN;
            processos[i].tested[j] = false;
        }
        processos[i].states[i] = CORRETO;
        processos[i].tested[i] = true;
    }
}

void escalona_sem_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 30.0, i);
    }
}

void escalona_rand(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 30.0, i);
    }
    for (int i = 0; i < N; i++) {
        if (randomic(0, 1) == 0) {
            schedule(FAULT, 31.0, i);
        }
    }
}

void escalona_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 30.0, i); // todos vao testar na unidade de tempo 30
    }
    for (int i = 1; i < N; i++) {
        schedule(FAULT, 31.0, i);
    }
}

void simula(int N, int max_unidades_tempo) {
    int token; // o processo com o token eh o que esta executando agora
    int event;
    while (time() < max_unidades_tempo) {
        cause(&event, &token);
        switch (event) {
            case TEST:
                if (status(processos[token].id) != 0) {
                    break; // processo falho nao testa
                }
                int prox = (token + 1) % N;
                while (status(processos[prox].id) != 0) {
                    printf("O processo %d testou o processo %d suspeito no tempo %4.1f\n", token, prox, time());
                    processos[token].states[prox] = FALHO;
                    processos[token].tested[prox] = true;
                    prox = (prox + 1) % N;
                }
                if (prox == token) {
                    printf("O processo %d testou todos os demais processos suspeitos no tempo %4.1f\n", token, time());
                } else {
                    printf("O processo %d testou o processo %d correto no tempo %4.1f\n", token, prox, time());
                    processos[token].states[prox] = CORRETO;
                    processos[token].tested[prox] = true;
                    printf("  obteve informacao sobre: [");
                    for (int i = 0; i < N; i++) {
                        if (!processos[token].tested[i] && processos[prox].states[i] != UNKNOWN) {
                            processos[token].states[i] = processos[prox].states[i];
                            printf("%2d ", i);
                        }
                    }
                    printf("]\n");
                }
                printf("    crenca do processo %d sobre os demais processos: [ ", token);
                for (int i = 0; i < N; i++) {
                    printf("%2d ", (int)processos[token].states[i]);
                }
                printf("]\n");
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
}

int main(int argc, char **argv) {
    char fa_name[5]; // facility name
    
    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <numero de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // numero de processos do sistema distribuido

    int MaxTempoSimulac = 30 * N;

    processos = malloc(sizeof(Processo) * N);
    assert(processos != NULL);
    for (int i = 0; i < N; i++) {
        processos[i].states = malloc(sizeof(State) * N);
        assert(processos[i].states != NULL);
        processos[i].tested = malloc(sizeof(bool) * N);
        assert(processos[i].tested != NULL);
    }

    printf("--------------------- teste: sem falhas -------------------\n");
    init_simulacao(N, fa_name);
    escalona_sem_falhas(N);
    simula(N, MaxTempoSimulac);
    
    printf("----------------- teste: falhas aleatorias ----------------\n");
    init_simulacao(N, fa_name);
    escalona_rand(N);
    simula(N, MaxTempoSimulac);

    printf("-------------- teste: todos os demais falham --------------\n");
    init_simulacao(N, fa_name);
    escalona_falhas(N);
    simula(N, MaxTempoSimulac);

    for (int i = 0; i < N; i++) {
        free(processos[i].states);
        free(processos[i].tested);
    }
    free(processos);

    return 0;
}
