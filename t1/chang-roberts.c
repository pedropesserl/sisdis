// Autor: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 21/05/2026
// Funcionalidade: Implementação do algoritmo de eleição de líder Chang-Roberts



// lista
// passo 1 FOI: variavel local com o id do lider
// passo 2 FOI: inicializacao das variaveis: -1 quando não é candidato, próprio ID quando é candidato
// passo 3 : Definir troca de mensagens entre processos: variável local? como fazer? primitivas? DEFINE SEND E RECV
// passo 4 : condicao de parada: a eleicao acaba quando o processo recebe devolta seu próprio ID do processo anterior
// passo 5 : escrever na tela quem é o lider
// passo 6 : contadores globais para número de rodadas e número de mensagens
// passo 7 : impressao do número de rodadas e número de mensagens



// LOGS
// logs para 1 candidato
// logs para vários candidatos selecionados aleatoriamente
// logs para todos os processos sendo candidatos





#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST     1
#define FAULT    2
#define RECOVERY 3
#define SEND 4
#define RECEIVE 5

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum {
    UNKNOWN = -1,
    CORRETO = 0,
    FALHO   = 1,
} State;

typedef struct
{
    int sender;
    int content;
} Message;

typedef struct {
    int id;        // identificador de facility do SMPL
    int leader_id;
    int received_leader; //buffer da mensagem recebida
    bool *tested;  // processos que testou nessa rodada
    State *states; // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;

void sort_single_candidate(int N)
{
  for(int i=0; i < N; i++)
  {
    processos[i].leader_id = -1;
  }
  processos[0].leader_id = processos[0].id;
}

void sort_random_candidates(int N)
{
  for(int i=0; i < N; i++)
  {
    if(randomic(0,1) == 0)
    {
      processos[i].leader_id = -1;
    }
    else
    {
      processos[i].leader_id = processos[i].id;
    }
  }
}

void sort_all_candidates(int N)
{
  for(int i=0; i < N; i++)
  {
    processos[i].leader_id = processos[i].id;
  }
}

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
    reset();
    stream(1);

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].id = 0;
        processos[i].received_message = -1;
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
        schedule(TEST, 1.0, i); // todos vao testar na unidade de tempo 30
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
                    printf("O processo %d testou o processo %d suspeito no tempo %4.1f\n", token, prox, time());
                    p->states[prox] = FALHO;
                    p->tested[prox] = true;
                    prox = (prox + 1) % N;
                }
                if (prox == token) {
                    printf("O processo %d testou todos os demais processos suspeitos no tempo %4.1f\n", token, time());
                } else {
                    printf("O processo %d testou o processo %d correto no tempo %4.1f\n", token, prox, time());
                    p->states[prox] = CORRETO;
                    p->tested[prox] = true;
                    printf("  obteve informacao sobre: [");
                    for (int i = 0; i < N; i++) {
                        if (!p->tested[i] && processos[prox].states[i] != UNKNOWN) {
                            p->states[i] = processos[prox].states[i];
                            printf("%2d ", i); //funciona, miraculosamente eu acho (questionamentos do pedrinho)
                        }
                    }
                    printf("]\n");

                    //funcao send message o pedrinho arruma depois
                    //invencao do pedrinho
                    processos[prox].received_leader = p->leader_id;
                    schedule(RECEIVE, 0.1, prox);


                }
                printf("    crenca do processo %d sobre os demais processos: [ ", token);
                for (int i = 0; i < N; i++) {
                    printf("%2d ", (int)p->states[i]);
                }
                printf("]\n");
                schedule(TEST, 1.0, token);
                break;
            case FAULT:
                request(p->id, token, 0);
                printf("O processo %d falhou no tempo %4.1f\n", token, time());
                break;
            case RECOVERY:
                release(p->id, token);
                printf("O processo %d recuperou no tempo %4.1f\n", token, time());
                schedule(TEST, 1.0, token);
                break;
            case RECEIVE: //UPON
                if (p->leader_id == p->received_leader) {
                  //nada acontece feijoada
                  printf("sou o lider\n");
                } else {
                  p->leader_id = MAX(p->leader_id, p->received_leader);
                }
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
    sort_single_candidate(N);
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
