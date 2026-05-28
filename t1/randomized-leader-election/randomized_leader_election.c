// Autores: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 22/05/2026
// Funcionalidade: Implementação do algoritmo de eleicao de lider Chang-Roberts

// lista
// passo 1 : variavel local com o id do lider
// passo 2 : variavel local com a decisao de ser candidato, pode ser 0 ou 1
// passo 3 : Criar um "vetor state" com os processos que tem o bit 1 
// passo 4 : repetir o processo
// passo 5 : quando o vetor tiver apenas 1 processo com o bit 1 esse processo se declara lider e se anuncia para todos os demais processos


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

typedef struct {
    int id;           // identificador de facility do SMPL
    int pid;
    int bit;
    int leader_id;
    int num_candidates;
    int *candidates;
    int msg;          // buffer da mensagem recebida
    bool *tested;     // processos que testou nessa rodada
    State *states;    // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;

void insere_ordenado(int vetor[], int *tam, int valor)
{
  int i = 0;
  while(i < *tam && vetor[i] < valor)
  {
    i++;
  }
  if(valor == vetor[i])
  {
    return;
  }
  for(int j = *tam; j > i; j--)
  {
    vetor[j] = vetor[j-1];
  }
  vetor[i] = valor;
  (*tam)++;
} //fim void

int esta_no_vetor(int vetor[], int *tam, int valor)
{
  for(int i = 0; i < *tam; i++)
  {
    if(vetor[i] == valor)
    {
      return 1;
    }
  }
  return 0;
} //fim int

//escolhe apenas um candidato, sendo ele o processo 0
void sort_single_candidate(int N) {
    for (int i = 0; i < N; i++) {
        processos[i].leader_id = -1;
    }
    processos[0].leader_id = 0;
    printf("O id do processo 0 eh: %d, seu leader_id eh: %d\n", processos[0].id, processos[0].leader_id);
}

void print_candidates(int N) {
    printf("Os candidatos a líder são: [");
    for (int i = 0; i < N; i++) {
        if (processos[i].bit == 1) {
            printf(" %2d", i);
        }
    }
    printf("]\n");
}

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Eleicao de lider: Algoritmo randomizado");
    reset();
    stream(1);

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].id = 0;
        processos[i].bit = randomic(0, 1);
        processos[i].msg = -1;
        if(processos[i].bit == 1)
        {
          insere_ordenado(processos[i].candidates, &(processos[i].num_candidates), i);
        }
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
        processos[i].states[i] = CORRETO;
        processos[i].tested[i] = true;
        processos[i].leader_id = -1;
    }
}

//inicia os eventos iniciais onde ninguem falha em nenhum momento
void escalona_sem_falhas(int N) {
    for (int i = 0; i < N; i++) {
        schedule(TEST, 1.0, i);
    }
}


//todo processo tem uma chance de 50% de falhar
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

//todos falham
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
                    printf("[%4.1f] O processo %d testou o processo %d correto\n", time(), token, prox);
                    p->states[prox] = CORRETO;
                    p->tested[prox] = true;
                    printf("       obteve informacao sobre: [");
                    for (int i = 0; i < N; i++) {
                        if (!p->tested[i] && processos[prox].states[i] != UNKNOWN) {
                            p->states[i] = processos[prox].states[i];
                            printf("%2d ", i);
                        }
                    }
                    printf("]\n");


                    // envia_mensagem se ja nao for lider
                    processos[prox].msg = p->leader_id;
                    schedule(RECEIVE, 0.1, prox);
                    printf("       O processo %d enviou uma mensagem com o id %d para o processo %d\n", token, p->leader_id, prox);
                    // fim_envia_mensagem
                }
                printf("       O que o processo %d sabe sobre todos os processos: [ ", token);
                for (int i = 0; i < N; i++) {
                    printf("%2d ", (int)p->states[i]);
                }
                printf("]\n");
                printf("       O buffer de mensagem de %d eh::: msg: %d \n\n", token, p->msg);
                schedule(TEST, 1.0, token);
                break;
            case FAULT:
                request(p->id, token, 0);
                printf("[%4.1f] O processo %d falhou\n", time(), token);
                break;

            case RECOVERY:
                release(p->id, token);
                printf("[%4.1f] O processo %d recuperou\n", time(), token);
                schedule(TEST, 1.0, token);
                break;
                
            //diferente do caso SEND, o RECEIVE eh um evento que deve ser tratado
            case RECEIVE:
                printf("o processo %d recebeu a mensagem")
                break;

            
            default:
                printf("fim da rodada %d\n\n", num_rodada);
                num_rodada++;
                break;
        } //fim_switch
    } //fim_while
} //fim_simula


//funcao main, inicia toda a simulacao e aloca os processos, bem como executa o algoritmo em si
int main(int argc, char **argv) {
    char fa_name[5]; // facility name

    if (argc != 2) {
        fprintf(stderr, "Uso correto: tempo <numero de processos>\n");
        return 1;
    }

    int N = atoi(argv[1]); // numero de processos do sistema distribuido

    int MaxTempoSimulac = 5;

    processos = malloc(sizeof(Processo) * N);
    assert(processos != NULL);
    for (int i = 0; i < N; i++) {
        processos[i].states = malloc(sizeof(State) * N);
        assert(processos[i].states != NULL);
        processos[i].tested = malloc(sizeof(bool) * N);
        assert(processos[i].tested != NULL);
        processos[i].candidates = malloc(sizeof(int) * N);
        assert(processos[i].candidates != NULL);
    }

    printf("--------------------- teste: sem falhas -------------------\n");
    init_simulacao(N, fa_name);
    sort_single_candidate(N);

    for(int i = 0; i < N; i ++)
    {
      printf("ID do processo %d : %d \n", i, processos[i].pid);
      printf("bit do processo %d: %d \n", i, processos[i].bit);
      printf("       O que o processo %d sabe sobre todos os processos: [ ", token);
      for (int i = 0; i < N; i++) {
          printf("%2d ", (int)p->states[i]);
      }
      printf("]\n");
    }
    print_candidates(N);
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
