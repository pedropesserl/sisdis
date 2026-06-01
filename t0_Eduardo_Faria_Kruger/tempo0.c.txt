/* 
  Autor: Eduardo Faria Kruger GRR20232329
  programa: tempo0.c
  Finalidade: aprender a programar a simuçação de algoritmos distribuídos pela aula gravada de sistemas distribuídos 
  Data da última alteração 30/05/2026
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
             event, r, i,
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

  //Vamos agora fazer o escalonamento dos eventos iniciais
  //No primeiro intervalo de testes os processos vão testar

  for(i=0; i<N; i++)
  {
    schedule(test, 30.0, i); //todos os processos de 0 até N-1 vão testar na unidade de tempo 30
  }
  printf("Log do trabalho prático 0 de Sistemas Distribuídos Prof. Elias\n");
  printf("Programa executado para N = 3 processos\n");
  printf("------------------------------tempo0.c-----------------------------\n");  
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
        printf("Sou o processo %d estou testando no tempo %4.1f\n", token, time());
        schedule(test, 30.0, token);
        break;
      case fault:
        r = request(processo[token].id, token, 0);
        printf("O processo %d falhou no tempo %4.1f\n", token, time());
        break;
      case recovery:
        release(processo[token].id, token);
        printf("O processo %d recuperou no tempo %4.1f\n", token, time());
        break;
      
    } //switch
  } // while
} //tempo.c

