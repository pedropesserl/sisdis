/*
  Autor: Eduardo Faria kruger GRR20232329
  programa: tempo4.c
  Finalidade: Fazer com que a cada teste teste correto, o processo testador obtenha todas as informações do vetor state do processo testado 
  Data da última modificação 30/05/2026
*/

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

//Vamos definir os eventos: um processo em algum instante de tempo pode sofrer um event
#define test 1
#define fault 2
#define recovery 3

#define UNKNOWN -1
#define CORRECT 0
#define FAILED 1


//vamos definir o descritor do processo

typedef struct {
  int id; //identificador de facility do SMPL
  int *state;
  //outras variáveis locais de cada processo são declaradas aqui!  
} TipoProcesso;



int main(int argc, char *argv[])
{
  static int N, //número de processos do sistema distribuído
             token, //indica o processo que está executando
             event, r, i, next,
             MaxTempoSimulac = 180;
  static char fa_name[5];
  TipoProcesso *processos;

  if(argc != 2)
  {
    puts("Uso correto: tempo <numero de processos>");
    exit(1);
  }
  N = atoi(argv[1]);

  printf("------------------------------tempo4.c-----------------------------\n");
  smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
  reset();
  stream(1);


  //inicializar os N processos
  TipoProcesso* initializeProcesses(int numProcess)
  {
    processos = (TipoProcesso *) malloc(sizeof(TipoProcesso)*numProcess);
    if(!processos)
    {
      return NULL;
    }
    for(int i=0; i<numProcess; i++)
    {
      processos[i].state = (int *) malloc(sizeof(int)*numProcess); //for apenas para alocar os vetores states e blablabla
      if(processos[i].state == NULL)
      {
        return NULL;
      }
      for(int j=0; j<numProcess; j++)
      {
        processos[i].state[j] = UNKNOWN;
      }
      processos[i].state[i] = CORRECT; //o processo sabe que ele mesmo está correto
      memset(fa_name, '\0', 5);
      sprintf(fa_name, "%d", i);
      processos[i].id = facility(fa_name, 1);     
    }
    return processos;
  }
  processos = initializeProcesses(N);

  void printStateVector(TipoProcesso ** processos, int numProcess, int id, int padding)
  {
    for(int pad=0; pad < padding; pad++) //padding function, usado só pra organizar os logs e deixar legível
    {
      printf(" ");
    }
    printf("Vetor State do processo %d: ", id);
    for(int j=0; j<numProcess; j++)
    {
      switch ((*processos)[id].state[j])
      {
        case FAILED:
          printf("F "); break;
        case CORRECT:
          printf("C "); break;
        case UNKNOWN:
          printf("U "); break;
        default:
          break;
      }
    }
    printf("\n");
  }
  for(int k=0; k<N; k++)
  {
    printStateVector(&processos, N, k, 0);
  }

  //Vamos agora fazer o escalonamento dos eventos iniciais
  //No primeiro intervalo de testes os processos vão testar
  for(i=0; i<N; i++)
  {
    schedule(test, 30.0, i); //todos os processos de 0 até N-1 vão testar o processo seguinte na unidade de tempo 30 
  }
  for(i=1; i<N; i++)
  {
    schedule(fault, 45.0, i); //todos os processos de 1 até N-1 vão falhar na unidade de tempo 30 
    schedule(recovery, 75.0, i); //todos os processos de 1 até N-1 vão recuperar na unidade de tempo 45
    schedule(test, 90.0, i); //todos os processos que falharam antes voltam a testar normal 
  }
  
  //agora vem o loop processo principal do simulador
  while (time() < MaxTempoSimulac)
  {
    cause(&event, &token);
    switch(event)
    {
      case test:
        if(status(processos[token].id) != 0)
        {
          break; //processo falho não testa e tambem nada acontece com seu vetor state, processo morto
        }
        printf("[%4.1f] Sou o processo %d estou testando no tempo %4.1f\n", time(), token, time());
        next = (token+1) % N;
        while(status(processos[next].id) != 0)
        {
          printf("       O processo %d testou o processo %d suspeito no tempo %4.1f\n", token, next, time());
          processos[token].state[next] = FAILED;
          printStateVector(&processos, N, token, 7); //imprime com padding de 13 pra facilitar na leitura dos logs, vai tudo ficar identado
          next = (next + 1) % N;
        }
        if(token == next)
        {
          printf("       O processo %d testou todos suspeitos e se testou correto\n", token); //nada acontece no vetor state, a entrada correspondente é  próprio processo, que está correto
          printStateVector(&processos, N, token, 7); //so imprime o vetor dai
        }
        else
        {
          printf("       O processo %d testou o processo %d correto\n", token, next);          
          memcpy(processos[token].state, processos[next].state, N * sizeof(int)); //obtem o vetor state do processo testado
          processos[token].state[token] = CORRECT; //marca esse processo como correto (pois testou correto)
          processos[token].state[next] = CORRECT; //se marca como correto (pois se penso logo existo)
          printStateVector(&processos, N, token, 7);          
        }
        schedule(test, 30.0, token);
        break;
      case fault:
        r = request(processos[token].id, token, 0);
        printf("[%4.1f] O processo %d falhou no tempo %4.1f\n", time(), token, time());
        break;
      case recovery:
        release(processos[token].id, token);
        printf("[%4.1f] O processo %d recuperou no tempo %4.1f\n",time(), token, time());
        break;
      
    } //switch
  } // while
} //tempo.c

