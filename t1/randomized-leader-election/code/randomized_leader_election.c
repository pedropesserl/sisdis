// Autores: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 22/05/2026
// Funcionalidade: Implementacao do algoritmo aleatorizado de eleicao de lider

// 1. Todos os processos sorteiam 1 bit
// 2. Repete ate ter um lider
//      2.1. Envia o bit atraves do anel
//              2.1.1. cada processo precisa receber mensagens de todos os outros
//      2.2. Se so tiver 1 bit 1, o processo que enviou e o lider
//      2.3. Senao, quem tem bit 1 sorteia de novo

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
#define RECEIVE  4

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
    int id;                 // identificador de facility do SMPL
    int pid;                // processId diferente do identificador do SMPL
    bool bit;               // bit sorteado nessa rodada
    int num_candidates;     // quantidade de processos que se candidataram nessa rodada
    int id_ult_candidato;   // id do ultimo processo candidato recebido
    Mensagem msg;           // buffer da mensagem recebida
    int num_msgs_esperadas; // quantidade de mensagens que espera receber
    int num_msgs_recebidas; // quantidade de mensagens que recebeu nessa rodada
    bool *tested;           // processos que testou nessa rodada
    State *states;          // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;

/* void insere_ordenado(int vetor[], int *tam, int valor) */
/* { */
/*   int i = 0; */
/*   while(i < *tam && vetor[i] < valor) */
/*   { */
/*     i++; */
/*   } */
/*   if(valor == vetor[i]) */
/*   { */
/*     return; */
/*   } */
/*   for(int j = *tam; j > i; j--) */
/*   { */
/*     vetor[j] = vetor[j-1]; */
/*   } */
/*   vetor[i] = valor; */
/*   (*tam)++; */
/* } //fim void */

/* int esta_no_vetor(int vetor[], int *tam, int valor) */
/* { */
/*   for(int i = 0; i < *tam; i++) */
/*   { */
/*     if(vetor[i] == valor) */
/*     { */
/*       return 1; */
/*     } */
/*   } */
/*   return 0; */
/* } //fim int */

void print_candidates(int N) {
    printf("Os candidatos a lider sao: [");
    for (int i = 0; i < N; i++) {
        if (processos[i].bit == 1) {
            printf(" %2d", i);
        }
    }
    printf("]\n");
}

void init_simulacao(int N, char fa_name[5]) {
    smpl(0, "Eleicao de lider: Algoritmo aleatorizado");
    reset();
    stream(1);

    // inicializar os N processos
    for (int i = 0; i < N; i++) {
        processos[i].num_candidates = 0;
        processos[i].id = 0;
        processos[i].bit = randomic(0, 1);
        processos[i].msg = (Mensagem){ .sender_id = -1, .bit = -1 };
        processos[i].num_msgs_esperadas = N;
        processos[i].num_msgs_recebidas = 0;
        processos[i].id_ult_candidato = -1;
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

                    processos[prox].msg = (Mensagem){ .sender_id = p->pid, .bit = p->bit };
                    schedule(RECEIVE, 0.1, prox);
                    printf("       O processo %d enviou a mensagem {bit: %d, pid: %d} para o processo %d\n", p->pid, p->bit, p->pid, prox);
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
                /* schedule(TEST, 1.0, token); */
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
                
            case RECEIVE:
                p->num_msgs_recebidas++;
                printf("[%4.1f] O processo %d recebeu a mensagem {bit: %d, pid: %d}\n", time(), p->pid, p->msg.bit, p->msg.sender_id);
                if (p->msg.sender_id != p->pid) { // nao eh a minha propria mensagem
                    // encaminhar mensagem
                    int prox = (token + 1) % N; // TODO: o que fazer se o proximo estiver falho?
                                                // tipo: da pra botar a mesma logica do TEST pra descobrir o proximo processo correto, mas ai o TEST existe justamente pra isso. so que nao da pra dar um schedule(TEST) aqui pq ele vai mandar a mensagem errada. ou seja ou repete tudo aqui ou sei la faz um evento diferente nao sei

                    // isso aqui ta errado. por exemplo: o processo 1 recebe uma mensagem do 0 e o 2 recebe do 1;
                    // mas o 1 encaminha pro 2 antes do 2 ter tempo de ler a mensagem que ele recebeu do 0;
                    // entao o 2 nunca le a mensagem do 0 e da tudo errado.
                    processos[prox].msg = p->msg;
                    schedule(RECEIVE, 0.1, prox);
                    printf("       O processo %d encaminhou a mensagem {bit: %d, pid: %d} para o processo %d\n", p->pid, p->msg.bit, p->msg.sender_id, prox);
                }
                if (p->msg.bit) {
                    p->num_candidates++;
                    p->id_ult_candidato = p->msg.sender_id;
                }
                printf("PROCESSO %d: num_msgs_recebidas = %d, num_msgs_esperadas = %d, num_candidates = %d\n", p->pid, p->num_msgs_recebidas, p->num_msgs_esperadas, p->num_candidates);
                if (p->num_msgs_recebidas == p->num_msgs_esperadas) { // acabou a rodada
                    if (p->num_candidates == 1) { // lider eleito
                        printf("[%4.1f] O processo %d elegeu o processo %d como lider\n", time(), p->pid, p->id_ult_candidato);
                        break;
                    } else { // proxima rodada
                        p->num_msgs_recebidas = 0;
                        p->id_ult_candidato = -1;
                        if (p->num_candidates == 0) { // deu errado
                            printf("PROCESSO %d DIZ: DEU ERRADO\n", p->pid);
                            /* p->num_msgs_esperadas = N; */
                            /* for (int i = 0; i < N; i++) { */
                            /*     processos[i].bit = randomic(0, 1); */
                            /*     schedule(TEST, 1.0, i); */
                            /* } */
                        } else {
                            p->num_msgs_esperadas = p->num_candidates;
                            p->num_candidates = 0;
                            if (p->bit) { // era candidato
                                p->bit = randomic(0, 1);
                                schedule(TEST, 1.0, p->pid);
                            }
                        }
                        print_candidates(N);
                    }
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

    int MaxTempoSimulac = 5;

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
