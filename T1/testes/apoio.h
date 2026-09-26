#pragma once
#include "../trabalho.h"

/* Funcoes reutilizadas pelos testes, escritas somente com pipes e sinais. */
typedef struct {
    int controle;                /* Escrita dos pedidos e das interrupcoes. */
    int resposta[QUANTIDADE_APLICACOES];
    int avisos;                  /* Cada auxiliar avisa quando recebe SIGCONT. */
    pid_t auxiliares[QUANTIDADE_APLICACOES];
    pid_t kernel;
} AmbienteTeste;

int iniciar_ambiente(AmbienteTeste *a);
void encerrar_ambiente(AmbienteTeste *a);
int enviar_pedido_teste(AmbienteTeste *a, int id, Operacao op, int pc, int n);
int enviar_irq_teste(AmbienteTeste *a, TipoIRQ irq);
int ler_resposta_teste(AmbienteTeste *a, int id, Operacao op, int n);
int esperar_ativo(AmbienteTeste *a, int id);
int verificar_parada(pid_t pid);
