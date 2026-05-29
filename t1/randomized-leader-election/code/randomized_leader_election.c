// Autores: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 29/05/2026
// Funcionalidade: Implementacao do algoritmo aleatorizado de eleicao de lider

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
    int sender_id;
    int bit;
} Mensagem;

typedef struct {
    int id;               // identificador de facility do SMPL
    int pid;              // processId diferente do identificador do SMPL
    bool bit;             // bit sorteado nessa rodada
    bool sent;            // indica se naquela rodada ele ja mandou seu bit ou se deve so repassar o que esta no buffer
    Mensagem msg;         // buffer da mensagem recebida
    Mensagem msg_to_send; // buffer de mensagens a enviar
    bool *candidates;     // processos que acredita serem candidatos
    bool *tested;         // processos que testou nessa rodada
    State *states;        // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;

int num_candidates(Processo *p, int N) {
    int contador = 0;
    for (int i = 0; i < N; i++) {
        if (p->candidates[i] && p->states[i] == CORRETO) {
            contador++; // conta somente os processos corretos
        }
    }
    return contador;
}

void print_candidates(int N) {
    printf("Os candidatos a lider sao: [");
    for (int i = 0; i < N; i++) {
        if (processos[i].bit == 1) {
            printf(" %d", i);
        }
    }
    printf(" ]\n");
}

void print_beliefs(int N, int i) {
  printf("Vetor de candidatos do processo %d: [", i);
  for (int j = 0; j < N; j++) {
      printf(" %d", processos[i].candidates[j]);
  }
  printf(" ]\n");
}

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Eleicao de lider: Algoritmo aleatorizado");
    reset();
    stream(1);

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].id = 0;
        processos[i].pid = i;
        processos[i].bit = randomic(0, 1);
        processos[i].msg = (Mensagem){ .sender_id = -1, .bit = -1 };
    }
    for (int i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%c", (char)i);
        processos[i].id = facility(fa_name, 1);
        for (int j = 0; j < N; j++) {
            processos[i].states[j] = UNKNOWN;
            processos[i].tested[j] = false;
            processos[i].candidates[j] = false;
        }
        processos[i].states[i] = CORRETO;
        processos[i].tested[i] = true;
        if (processos[i].bit == 1) {
            processos[i].candidates[i] = true;
        }
    }
    for (int i = 0; i < N; i++) {
        print_beliefs(N, i);
    }
    print_candidates(N);
}

void escalona_sem_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 1.0, i);
    }
}

void escalona_rand(int N) {
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
    int num_rodada = 1;
    int num_mensagens = 0;
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


                    // envia mensagem
                    if (p->sent) { // se ja mandou o seu bit, so repassa a mensagem e mantem sent = true
                        processos[prox].msg = processos[token].msg_to_send;
                        printf("       O processo %d repassou a mensagem {bit: %d , pid: %d} para o processo %d\n", p->pid, p->msg_to_send.bit, p->msg_to_send.sender_id, prox);
                    } else { // se ainda nao mandou o seu bit, manda o seu bit e seta sent para true
                        processos[prox].msg = (Mensagem){ .sender_id = p->pid, .bit = p->bit };
                        p->sent = true; // aqui eu mandei meu id e o meu bit pra frente
                        printf("       O processo %d enviou a mensagem {bit: %d, pid: %d} para o processo %d\n", p->pid, p->bit, p->pid, prox);
                    }
                    schedule(RECEIVE, 0.1, prox);
                    // fim envia mensagem
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
                printf("       "); print_beliefs(N, token);
                schedule(TEST, 1.0, token);
                break;

            case FAULT:
                request(p->id, token, 0);
                printf("[%4.1f] O processo %d falhou\n", time(), token);
                break;
                
            case RECEIVE:
                printf("[%4.1f] O processo %d recebeu a mensagem {bit: %d, pid: %d}\n", time(), p->pid, p->msg.bit, p->msg.sender_id);
                num_mensagens++;
                if (p->msg.sender_id != p->pid) { // nao eh a minha propria mensagem, so encaminhar mensagem
                    processos[token].msg_to_send = p->msg;
                    p->candidates[p->msg.sender_id] = p->msg.bit;
                    break;
                }
                // recebi minha propria mensagem: fim da rodada
                num_rodada++;
                p->sent = false;
                printf("       O processo %d terminou uma rodada\n", p->pid);
                int nc = num_candidates(p, N);
                if (nc == 0) { // falhou
                    printf("       O processo %d detectou que NAO HA NENHUM candidato: sorteando bit novamente\n", p->pid);
                    int novo_bit = randomic(0,1);
                    printf("       O processo %d tinha o bit = %d e agora tem o bit = %d \n", p->pid, p->bit, novo_bit);
                    p->bit = novo_bit;
                    p->candidates[p->pid] = p->bit;
                } else if (nc == 1) { // lider eleito
                    if (p->bit == 0) {
                        break; // nao sou o lider eleito
                    }
                    printf("[%4.1f] O processo %d foi eleito o lider do sistema\n", time(), p->pid);
                    printf("FIM DO ALGORITMO (tempo: %4.1f).\n", time());
                    for (int i = 0; i < N; i++) {
                        if (status(processos[i].id) != 0) {
                            printf("O processo %d esta falho\n", i);
                        } else {
                            print_beliefs(N, i);
                        }
                    }
                    printf("Total de mensagens transmitidas na execucao do algoritmo: %d\n", num_mensagens);
                    printf("Total de rodadas necessarias: %d\n", num_rodada);
                    return; // fim do algoritmo
                } else {
                    if (p->bit == 1) {
                        int novo_bit = randomic(0,1);
                        printf("       O processo %d tinha o bit = %d e agora tem o bit = %d\n", p->pid, p->bit, novo_bit);
                        p->bit = novo_bit;
                        p->candidates[p->pid] = p->bit;
                    }

                }
                if (p->pid == 0) { // ultimo processo terminou a rodada
                    print_candidates(N);    
                }
                break;
            default:
                break;
        }
    }
}

//funcao main, inicia toda a simulacao e aloca os processos, e executa o algoritmo
int main(int argc, char **argv) {
    char fa_name[5]; // facility name

    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <numero de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // numero de processos do sistema distribuido

    int MaxTempoSimulac = 50;

    processos = malloc(sizeof(Processo) * N);
    assert(processos != NULL);
    for (int i = 0; i < N; i++) {
        processos[i].states = malloc(sizeof(State) * N);
        assert(processos[i].states != NULL);
        processos[i].tested = malloc(sizeof(bool) * N);
        assert(processos[i].tested != NULL);
        processos[i].candidates = malloc(sizeof(bool) * N);
    }

    printf("--------------------- teste: sem falhas -------------------\n");
    init_simulacao(N, fa_name);
    escalona_sem_falhas(N);
    simula(N, MaxTempoSimulac);

    //    printf("----------------- teste: falhas aleatorias ----------------\n");
    //    init_simulacao(N, fa_name);
    //    escalona_rand(N);
    //    simula(N, MaxTempoSimulac);

    //    printf("-------------- teste: todos os demais falham --------------\n");
    //    init_simulacao(N, fa_name);
    //    escalona_falhas(N);
    //    simula(N, MaxTempoSimulac);

    for (int i = 0; i < N; i++) {
        free(processos[i].states);
        free(processos[i].tested);
    }
    free(processos);

    return 0;
}
