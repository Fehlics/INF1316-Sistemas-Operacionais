#include "simulador_pausa.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int enviar_evento(int fd, TipoEvento tipo) {
    MensagemControle mensagem = {.tipo = tipo};
    return enviar_dados(fd, &mensagem, sizeof mensagem);
}

static const char *nome_estado(EstadoProcesso estado) {
    switch (estado) {
        case PRONTO: return "PRONTO";
        case EXECUTANDO: return "EXECUTANDO (suspenso na pausa)";
        case BLOQUEADO_LEITURA:
        case BLOQUEADO_ESCRITA: return "BLOQUEADO";
        case TERMINADO: return "TERMINADO";
    }
    return "DESCONHECIDO";
}

static void mostrar_estados(const EstadoSimulador *e) {
    printf("\n[Simulador] ===== ESTADO DOS PROCESSOS (PAUSADO) =====\n");
    printf("[Simulador] CPU no instante da pausa: %s",
           e->executando == 0 ? "nenhuma aplicacao" : "A");
    if (e->executando) printf("%d", e->executando);
    putchar('\n');
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        const Processo *p = &e->processos[i];
        printf("[Estado] A%d PID=%ld PC=%d N=%d ESTADO=%s",
               p->id, (long)p->pid, p->pc, p->n, nome_estado(p->estado));
        if (p->operacao_pendente != NENHUMA_OPERACAO) {
            printf(" DISPOSITIVO=pipe%d OPERACAO=%s ENDERECO=%s",
                   i / 2 + 1, p->operacao_pendente == ENVIAR ? "SEND" : "RECV",
                   p->endereco_pendente == ENDERECO_PC ? "PC" : "N");
        } else printf(" DISPOSITIVO=nenhum OPERACAO=nenhuma ENDERECO=nenhum");
        printf(" EXECUTANDO=%s LEITURAS=%d ESCRITAS=%d TERMINADO=%s\n",
               p->estado == EXECUTANDO ? "sim (antes da pausa)" : "nao",
               p->leituras, p->escritas,
               p->estado == TERMINADO ? "sim" : "nao");
    }
    puts("[Simulador] Ctrl+Z novamente para retomar.\n");
}

/* Pausa primeiro o controlador para que nenhuma IRQ chegue apos o aviso. */
int pausar_sistema(int controle, int fd_estado, pid_t controlador,
                  pid_t aplicativos[], int *restantes,
                  volatile sig_atomic_t *encerrar) {
    if (controlador > 0) {
        if (kill(controlador, SIGSTOP) < 0) return 0;
        int situacao;
        pid_t retorno;
        do { retorno = waitpid(controlador, &situacao, WUNTRACED); }
        while (retorno < 0 && errno == EINTR && !*encerrar);
        if (retorno != controlador || !WIFSTOPPED(situacao)) return 0;
    }
    EstadoSimulador e;
    if (!enviar_evento(controle, EVENTO_PAUSAR) ||
        !receber_dados(fd_estado, &e, sizeof e) || e.fase != 1) return 0;

    int i = e.executando - 1;
    if (i >= 0 && i < QUANTIDADE_APLICACOES && aplicativos[i] > 0) {
        for (int tentativa = 0; tentativa < 50; tentativa++) {
            int situacao;
            pid_t retorno = waitpid(aplicativos[i], &situacao,
                                    WUNTRACED | WNOHANG);
            if (retorno == aplicativos[i]) {
                if (WIFEXITED(situacao) || WIFSIGNALED(situacao)) {
                    aplicativos[i] = 0;
                    if (--*restantes == 0)
                        puts("[Simulador] As seis aplicacoes foram recolhidas.");
                }
                break;
            }
            if (retorno < 0 && errno != EINTR) break;
            usleep(10000);
        }
    }
    /* A fotografia seguinte inclui mensagens de contexto anteriores ao STOP. */
    if (!enviar_evento(controle, EVENTO_MOSTRAR) ||
        !receber_dados(fd_estado, &e, sizeof e) || e.fase != 2) return 0;
    mostrar_estados(&e);
    return 1;
}

int retomar_sistema(int controle, int fd_estado, pid_t controlador) {
    EstadoSimulador e;
    if (!enviar_evento(controle, EVENTO_RETOMAR) ||
        !receber_dados(fd_estado, &e, sizeof e) || e.fase != 3) return 0;
    if (controlador > 0) kill(controlador, SIGCONT);
    puts("[Simulador] Execucao retomada. Ctrl+Z para pausar novamente.");
    return 1;
}
