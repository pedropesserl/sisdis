/*
  Autor: Eduardo Faria Kruger GRR20232329
  programa: tempo1.c
  Finalidade: Fazer cada processo testar o próximo no anel
  Data da última modificação 30/05/2026
*/

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

//Vamos definir os eventos: um processo em algum instante de tempo pode sofrer um event
#define test 1
#define fault 2
#define recovery 3

//vamos definir o descritor do processo

typedef struct {
  int id; //identificador de facility do SMPL
  //outras variáveis locais de cada processo são declaradas aqui!  
} TipoProcesso;


TipoProcesso *processo;

int main(int argc, char *argv[])
{
  static int N, //número de processos do sistema distribuído
             token, //indica o processo que está executando
             event, i, r, next,
             MaxTempoSimulac = 180;
  static char fa_name[5];

  if(argc != 2)
  {
    puts("Uso correto: tempo <numero de processos>");
    exit(1);
  }
  N = atoi(argv[1]);

  smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
  reset();
  stream(1);

  //inicializar os N processos

  processo = (TipoProcesso *) malloc(sizeof(TipoProcesso)*N);

  for(i=0; i<N; i++)
  {
    memset(fa_name, '\0', 5);
    sprintf(fa_name, "%d", i);
    processo[i].id = facility(fa_name, 1);    
  }

  //agendamento dos eventos iniciais. Faults precisam re recover (ou não), teste se auto agenda depois
  
  for(i=0; i<N; i++)
  {
    schedule(test, 30.0, i); //todos os processos de 0 até N-1 vão testar o processo seguinte na unidade de tempo 30 
  }
  for(i=1; i<N; i++)
  {
    schedule(fault, 45.0, i); //todos os processos de 0 até N-1 vão falhar no tempo 45
    schedule(recovery, 75.0, i); //todos os processos de 0 até N-1 vao recuperar no tempo 75
    schedule(test, 90, i); //todos que tinham falhado voltam a testar
  }
  
  printf("------------------------------tempo1.c-----------------------------\n");
  //agora vem o loop processo principal do simulador
  while (time() < MaxTempoSimulac)
  {
    cause(&event, &token);
    switch(event)
    {
      case test:
        if(status(processo[token].id) != 0)
        {
          break; //processo falho não testa
        }
        printf("[%4.1f] Sou o processo %d estou testando\n", time(), token);

        //endereço do próximo processo no anel
        next = (token+1) % N;

        //teste acontecendo
        if(status(processo[next].id) != 0)
          printf("       O processo %d testou o processo %d suspeito\n", token, next);
        else 
          printf("       O processo %d testou o processo %d correto \n", token, next);
        schedule(test, 30.0, token); //agenda o próximo teste
        break;
      
      case fault:
        r = request(processo[token].id, token, 0);
        printf("[%4.1f] O processo %d falhou\n", time(), token);
        break;
      case recovery:
        release(processo[token].id, token);
        printf("[%4.1f] O processo %d recuperou\n", time(), token);
        break;
      
    } //switch
  } // while
} //tempo.c


