// Autores: Pedro Folloni Pesserl GRR20220072 && Eduardo Faria Kruger GRR20232329
// Data ultima modificacao: 29/05/2026
// Funcionalidade: Implementacao do algoritmo aleatorizado de eleicao de lider

// 1. Todos os processos sorteiam 1 bit
// 2. Repete ate ter um lider
//      2.1. Envia o bit atraves do anel
//              2.1.1. cada processo precisa receber mensagens de todos os outros
//      2.2. Se so tiver 1 bit 1, o processo que enviou e o lider
//      2.3. Senao, quem tem bit 1 sorteia de novo

//primeira rodada todos os processos esperam receber N-1 mensagens
//da segunda rodada em diante todos os processos esperam recebe
//Então cada processo deve guardar uma estrutura de dados que representa (olha, estou mensando a minha mensagem e passando a mensagem dos outros procesoss para frente também)
//essa lógica é tratada no evento RECEIVE, que altera a estrutura


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
#define RECEIVE  3

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum {
    UNKNOWN = -1,
    CORRETO = 0,
    FALHO   = 1,
} State;

typedef enum {
    CANDIDATE = 1,
    NOT_CANDIDATE = 0,
} Decision;

typedef struct {
    int sender_id;
    int bit;
} Mensagem;

typedef struct {
    int id;                            // identificador de facility do SMPL
    int pid;                           // processId diferente do identificador do SMPL
    bool bit;                          // bit sorteado nessa rodada
    bool sent;                         //indica se naquela rodada ele ja mandou seu bit ou se deve so repassar o que esta no buffer
    int num_candidates;                // quantidade de processos que se candidataram nessa rodada
    int id_ult_candidato;              // id do ultimo processo candidato recebido
    Mensagem msg;                      // buffer da mensagem recebida
    Mensagem msg_to_send;
    Decision *decisions;
    int num_msgs_esperadas;            // quantidade de mensagens que espera receber
    int num_msgs_recebidas;            // quantidade de mensagens que recebeu nessa rodada
    bool *tested;                      // processos que testou nessa rodada
    State *states;                     // crenca do processo a respeito dos estados dos demais
} Processo;

Processo *processos;

int qty_number_ones(Processo *processo_que_chama, int tam)
{
  int contador = 0;
  for(int i = 0; i < tam; i++)
  {
    if((processo_que_chama->decisions[i] == 1) && processo_que_chama->states[i] == CORRETO) contador++; //conta somente os processos corretos
  }
  return contador;
}

void print_candidates(int N) {
    printf("Os candidatos a lider sao: [");
    for (int i = 0; i < N; i++) {
        if (processos[i].bit == 1) {
            printf(" %2d", i);
        }
    }
    printf("]\n");
}

void print_beliefs(int N, int i)
{
  printf("O processo %d tem a seguinte crença: ", i);
  printf("[ ");
  for(int j = 0; j < N; j++)
  {
    printf("%d ", processos[i].decisions[j]);
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
            processos[i].decisions[j] = NOT_CANDIDATE;
        }
        if(processos[i].bit == 1) processos[i].decisions[i] = CANDIDATE;
        processos[i].pid = i;
        processos[i].states[i] = CORRETO;
        processos[i].tested[i] = true;
    }
    for(int i = 0; i < N; i++)
    {
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


                    //envia_mensagem
                    if(p->sent) {  //se ja mandou o seu bit, só repassa a mensagem e mantem sent = true
                      processos[prox].msg = processos[token].msg_to_send;
                      printf("       O processo %d repassou a mensagem {bit: %d , pid: %d} para o processo %d\n", p->pid, p->msg_to_send.bit, p->msg_to_send.sender_id, prox);
                    }
                    else{ //se ainda nao mandou o seu bit, manda o seu bit e seta sent para true
                      processos[prox].msg = (Mensagem){ .sender_id = p->pid, .bit = p->bit };
                      p->sent = true; //aqui eu mandei meu id e o meu bit pra frente
                      printf("       O processo %d enviou a mensagem {bit: %d, pid: %d} para o processo %d\n", p->pid, p->bit, p->pid, prox);
                    }
                    schedule(RECEIVE, 0.1, prox);
                    //fim_envia_mensagem
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
                p->num_msgs_recebidas++;
                num_mensagens++;
                if (p->msg.sender_id != p->pid) { // nao eh a minha propria mensagem, se nao eh minha mensagem eh só encaminhar mensagem
                    processos[token].msg_to_send = p->msg;
                    p->decisions[p->msg.sender_id] = p->msg.bit;
                    //isso aqui vai ficar guardado para a próxima vez que testar um processo correto, dai ele vai enviar
                    
                    //schedule(RECEIVE, 0.1, prox); ele nao vai fazer schedule, confia
                    //printf("       O processo %d encaminhou a mensagem {bit: %d, pid: %d} para o processo %d\n", p->pid, p->msg.bit, p->msg.sender_id, prox);
                }
                
                if (p->msg.sender_id == p->pid)
                {
                  num_rodada++;
                  p->sent = false; //AQUI A RODADA ACABA RAPAZ (quando a mensagem que ele enviou deu toda a volta e obrigatoriamente passou por todos os processos corretos
                  printf("-------------------------AQUI UMA RODADA ACABOU-------------------------------------\n");
                  int qty_ones = qty_number_ones(p, N);
                  switch (qty_ones){
                    case 0:
                      int novo_bit = randomic(0,1);
                      printf("Aqui pelo acaso do destino ninguém quis se candidatar, então vamos todos sortear novamente\n");
                      printf("O processo %d tinha o bit = %d e agora tem o bit = %d \n", p->pid, p->bit, novo_bit);
                      p->bit = novo_bit;
                      if (!p->bit) p->decisions[p->pid] = NOT_CANDIDATE;  
                    case 1:
                      if (p->bit == 1) {
                        printf("[%4.1f] O processo %d foi eleito o lider do sistema\n", time(), p->pid);
                        printf("FIM DO ALGORITMO (tempo: %4.1f).\n", time());
                        for (int i = 0; i < N; i++) {
                            if (status(processos[i].id) != 0) {
                              printf("O processo %d esta falho\n", i);
                            }
                            else {
                              print_beliefs(N, i);
                            }
                        }
                        printf("Total de mensagens transmitidas na execucao do algoritmo: %d\n", num_mensagens);
                        printf("Total de rodadas necessárias: %d\n", num_rodada);
                        return; // fim do algoritmo
                      }
                    default:
                      if(p->bit == 1)
                      {
                        int novo_bit = randomic(0,1);
                        printf("O processo %d tinha o bit = %d e agora tem o bit = %d \n", p->pid, p->bit, novo_bit);
                        p->bit = novo_bit;
                        if (!p->bit) p->decisions[p->pid] = NOT_CANDIDATE;
                      }
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

    int MaxTempoSimulac = 50;

    processos = malloc(sizeof(Processo) * N);
    assert(processos != NULL);
    for (int i = 0; i < N; i++) {
        processos[i].states = malloc(sizeof(State) * N);
        assert(processos[i].states != NULL);
        processos[i].tested = malloc(sizeof(bool) * N);
        assert(processos[i].tested != NULL);
        processos[i].decisions = malloc(sizeof(Decision) * N);
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
