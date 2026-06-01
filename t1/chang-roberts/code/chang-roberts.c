// Autores: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 29/05/2026
// Funcionalidade: Implementacao do algoritmo de eleicao de lider Chang-Roberts

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST     1
#define FAULT    2
#define RECEIVE  3

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum {
    UNKNOWN = -1,
    CORRETO = 0,
    FALHO   = 1,
} State;

typedef struct {
    int id;              // identificador de facility do SMPL
    int pid;             // processId diferente do identificador do SMPL
    int leader_id;       // atual candidato a lider deste processo
    int msg;             // buffer da mensagem recebida
    bool am_i_candidate; // se o processo remetente eh lider
    bool *tested;        // processos que testou nessa rodada
    State *states;       // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;
int total_mensagens;

void print_candidates(int N) {
    printf("Os candidatos a lider sao: [");
    for (int i = 0; i < N; i++) {
        if (processos[i].leader_id == processos[i].pid) {
            printf(" %2d", i);
        }
    }
    printf("]\n");
}

// escolhe apenas um candidato, sendo ele o processo 0
void sort_single_candidate(int N) {
    for (int i = 0; i < N; i++) {
        processos[i].leader_id = -1;
    }
    processos[0].leader_id = 0;
    print_candidates(N);
}

// escolhe candidatos aleatoriamente
void sort_random_candidates(int N) {
    for (int i = 0; i < N; i++) {
        if (randomic(0,1) == 0) {
            processos[i].leader_id = -1;
        } else {
            processos[i].leader_id = processos[i].pid;
        }
    }
    print_candidates(N);
}

// todo mundo eh candidato
void sort_all_candidates(int N) {
    for (int i = 0; i < N; i++) {
        processos[i].leader_id = processos[i].pid;
    }
    print_candidates(N);
}

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Eleicao de lider: Chang-Roberts");
    reset();
    stream(1);
    total_mensagens = 0;

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].id = 0;
        processos[i].msg = -1;
    }
    for (int i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%c", (char)i);
        processos[i].id = facility(fa_name, 1);
        for (int j = 0; j < N; j++) {
            processos[i].states[j] = UNKNOWN;
            processos[i].tested[j] = false;
        }
        processos[i].pid = i;
        processos[i].am_i_candidate = false;
        processos[i].states[i] = CORRETO;
        processos[i].tested[i] = true;
        processos[i].leader_id = -1;
    }
}

void escalona_sem_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 1.0, i);
    }
}

void escalona_randomic(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 1.0, i);
    }
    for (int i = 0; i < N; i++) {
        if (randomic(0, 1) == 0) {
            schedule(FAULT, 2.0, i);
        }
    }
}

void escalona_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 1.0, i);
    }
    for (int i = 1; i < N; i++) {
        schedule(FAULT, 2.0, i);
    }
}

void simula(int N, int max_unidades_tempo) {
    int token; // o processo com o token eh o que esta executando agora
    int event;
    while (time() < max_unidades_tempo) {
        cause(&event, &token);
        Processo *p = &processos[token];
        switch (event) {
            case TEST:
                if (status(p->id) != 0) {
                    break; // processo falho nao testa
                }
                int prox = (token + 1) % N;
                while (status(processos[prox].id) != 0) {
                    printf("[%4.1f] O processo %d testou o processo %d suspeito\n", time(), token, prox);
                    p->states[prox] = FALHO;
                    p->tested[prox] = true;
                    prox = (prox + 1) % N;
                }
                if (prox == token) {
                    printf("[%4.1f] O processo %d testou todos os demais processos suspeitos\n", time(), token);
                    
                    //envia_mensagem
                    processos[prox].msg = p->leader_id;
                    processos[prox].am_i_candidate = p->pid == p->leader_id;
                    schedule(RECEIVE, 0.1, prox); //agenda um evento para si mesmo, apenas cai no caso que já tinhamos tratado
                    printf("       O processo %d enviou uma mensagem com o id %d para o processo %d\n", token, p->leader_id, prox);
                    //fim_envia_mensagem

                    
                } else {
                    p->states[prox] = CORRETO;
                    p->tested[prox] = true;
                    printf("[%4.1f] O processo %d testou o processo %d correto e obteve informacao sobre: [", time(), token, prox);
                    for (int i = 0; i < N; i++) {
                        if (!p->tested[i] && processos[prox].states[i] != UNKNOWN) {
                            p->states[i] = processos[prox].states[i];
                            printf("%2d ", i);
                        }
                    }
                    printf("]\n");

                    // envia_mensagem
                    processos[prox].msg = p->leader_id;
                    processos[prox].am_i_candidate = p->pid == p->leader_id;
                    schedule(RECEIVE, 0.1, prox);
                    printf("       O processo %d enviou uma mensagem com o id %d para o processo %d\n", token, p->leader_id, prox);
                    // fim_envia_mensagem
                }
                printf("       Vetor de estados do processo %d: [ ", token);
                for (int i = 0; i < N; i++) {
                    switch (p->states[i]) {
                        case UNKNOWN:
                            printf("U ");
                            break;
                        case CORRETO:
                            printf("C ");
                            break;
                        case FALHO:
                            printf("S "); // suspeito
                    }
                }
                printf("]\n");
                schedule(TEST, 1.0, token);
                break;

            case FAULT:
                request(p->id, token, 0);
                printf("[%4.1f] O processo %d falhou\n", time(), token);
                break;

            case RECEIVE:
                total_mensagens++;
                printf("[%4.1f] O processo %d recebeu uma mensagem com id: %d com am_i_candidate: %d\n", time(), p->pid, p->msg, p->am_i_candidate);
                if ((p->pid == p->leader_id) && (p->leader_id == p->msg)) {
                    printf("[%4.1f] O processo %d foi eleito o lider do sistema\n", time(), token);
                    printf("FIM DO ALGORITMO (tempo: %4.1f).\n", time());
                    for (int i = 0; i < N; i++) {
                        if (status(processos[i].id) != 0) {
                            printf("O processo %d esta falho\n", i);
                        } else {
                            printf("O processo %d acredita que o processo %d eh o lider\n", i, processos[i].leader_id);
                        }
                    }
                    printf("Total de mensagens transmitidas na execucao do algoritmo: %d\n", total_mensagens);
                    return; // fim do algoritmo
                } else {
                    p->leader_id = MAX(p->leader_id, p->msg);
                }
                break;
            default:
                break;
        }
    }
}


// funcao main, inicia toda a simulacao e aloca os processos, e executa o algoritmo
int main(int argc, char **argv) {
    char fa_name[5]; // facility name

    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <numero de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // numero de processos do sistema distribuido

    int MaxTempoSimulac = 120;

    processos = malloc(sizeof(Processo) * N);
    assert(processos != NULL);
    for (int i = 0; i < N; i++) {
        processos[i].states = malloc(sizeof(State) * N);
        assert(processos[i].states != NULL);
        processos[i].tested = malloc(sizeof(bool) * N);
        assert(processos[i].tested != NULL);
    }

    printf("---- teste: apenas um candidato -----------------------------------\n");
    init_simulacao(N, fa_name);
    sort_single_candidate(N);
    escalona_sem_falhas(N);
    simula(N, MaxTempoSimulac);

    printf("\n---- teste: candidatos aleatorios -------------------------------\n");
    init_simulacao(N, fa_name);
    sort_random_candidates(N);
    escalona_sem_falhas(N);
    simula(N, MaxTempoSimulac);

    printf("\n---- teste: todos sao candidatos --------------------------------\n");
    init_simulacao(N, fa_name);
    sort_all_candidates(N);
    escalona_sem_falhas(N);
    simula(N, MaxTempoSimulac);

    printf("\n---- teste: apenas um candidato com falhas aleatórias -----------\n");
    init_simulacao(N, fa_name);
    sort_single_candidate(N);
    escalona_randomic(N);
    simula(N, MaxTempoSimulac);

    for (int i = 0; i < N; i++) {
        free(processos[i].states);
        free(processos[i].tested);
    }
    free(processos);

    return 0;
}
