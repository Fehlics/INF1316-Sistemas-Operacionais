#include "testes.h"
#include "apoio.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* Confere o aviso REAL de uma Application, com apenas tres iteracoes.
 * O limite reduzido existe somente no modo de teste: normalmente sao 5000. */
static int testar_aviso_da_aplicacao(void) {
    int controle[2], resposta[2];
    if (pipe(controle) == -1) return 0;
    if (pipe(resposta) == -1) {
        close(controle[0]); close(controle[1]);
        return 0;
    }

    pid_t filho = fork();
    if (filho == -1) {
        close(controle[0]); close(controle[1]);
        close(resposta[0]); close(resposta[1]);
        return 0;
    }
    if (filho == 0) {
        close(controle[0]); close(resposta[1]);
        FILE *saida = fopen("/dev/null", "w");
        if (saida != NULL) {
            dup2(fileno(saida), STDOUT_FILENO);
            fclose(saida);
        }
        char fd_controle[16], fd_resposta[16];
        snprintf(fd_controle, sizeof fd_controle, "%d", controle[1]);
        snprintf(fd_resposta, sizeof fd_resposta, "%d", resposta[0]);
        setenv("TESTE_MAX_ITERACOES", "3", 1);
        setenv("TESTE_SEM_SYSCALL", "1", 1);
        execl("./Application", "Application", "3", fd_controle,
              fd_resposta, (char *)NULL);
        _exit(127);
    }
    close(controle[1]); close(resposta[0]);

    MensagemControle aviso;
    int ok = 0, passos = 0, mensagens_inteiras = 0;
    for (;;) {
        size_t total = 0;
        while (total < sizeof aviso) {
            ssize_t n = read(controle[0], (char *)&aviso + total,
                             sizeof aviso - total);
            if (n == 0) break;
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            total += (size_t)n;
        }
        if (total != sizeof aviso) break;
        if (aviso.tipo != EVENTO_CONTEXTO) {
            mensagens_inteiras = 1;
            break;
        }
        passos++;
        if (aviso.pedido.pc != passos || passos > 3) break;
    }
    if (passos == 3 && mensagens_inteiras &&
        aviso.tipo == EVENTO_TERMINO &&
        aviso.pedido.id_aplicacao == 3 && aviso.pedido.pc == 3 &&
        aviso.pedido.n == 0 && aviso.pedido.operacao == NENHUMA_OPERACAO) {
        int status;
        /* A aplicacao so pode encerrar depois que o kernel confirmar. */
        if (waitpid(filho, &status, WNOHANG) == 0) {
            RespostaSyscall confirmacao = {3, NENHUMA_OPERACAO, 0};
            if (write(resposta[1], &confirmacao, sizeof confirmacao) ==
                (ssize_t)sizeof confirmacao) {
                for (int i = 0; i < 100; i++) {
                    pid_t r = waitpid(filho, &status, WNOHANG);
                    if (r == filho) {
                        filho = -1;
                        ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
                        break;
                    }
                    if (r < 0) break;
                    usleep(10000);
                }
            }
        }
    }
    if (filho > 0) {
        kill(filho, SIGCONT);
        kill(filho, SIGTERM);
        waitpid(filho, NULL, 0);
    }
    close(controle[0]); close(resposta[1]);
    if (ok) puts("[OK] Application completou 3 iteracoes e aguardou confirmacao.");
    else puts("[ERRO] Aviso de termino da Application.");
    return ok;
}

/* Auxiliares simulam processos, mas nao executam a logica da Application.
 * Ao registrar seu termino, nos os recolhemos para emular a saida real. */
static void recolher_auxiliar(AmbienteTeste *a, int id) {
    pid_t pid = a->auxiliares[id - 1];
    if (pid > 0) {
        /* Este auxiliar ja esta executando: o KernelSim enviou
         * SIGCONT antes da confirmacao do termino. Outro SIGCONT
         * geraria um aviso duplicado e alteraria a ordem dos testes. */
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        a->auxiliares[id - 1] = -1;
    }
}

static int terminar_ativo(AmbienteTeste *a, int id, int n, int proximo) {
    if (!enviar_termino_teste(a, id, MAX_ITERACOES, n) ||
        !ler_resposta_teste(a, id, NENHUMA_OPERACAO, n)) return 0;
    if (proximo > 0 && !esperar_ativo(a, proximo)) return 0;
    recolher_auxiliar(a, id);
    return 1;
}

static int testar_registro_e_escalonamento(void) {
    AmbienteTeste a;
    int ok = 0;
    if (!iniciar_ambiente(&a)) return 0;

    /* A1 esta executando: seu termino deve liberar A2 imediatamente. */
    if (!terminar_ativo(&a, 1, 41, 2)) goto fim;
    puts("[OK] KernelSim reconheceu A1 e escalonou A2.");

    /* A4 foi preemptada antes do aviso: precisa de SIGCONT apenas
       para receber a confirmacao e encerrar, nao para voltar a fila. */
    if (!enviar_termino_teste(&a, 4, MAX_ITERACOES, 44) ||
        !ler_resposta_teste(&a, 4, NENHUMA_OPERACAO, 44) ||
        !esperar_ativo(&a, 4)) goto fim;
    recolher_auxiliar(&a, 4);
    puts("[OK] Aviso de termino recebido de uma aplicacao suspensa.");

    /* Rodada completa: nao se deve escalonar A1 nem A4 novamente. */
    if (!enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 3) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 5) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 6) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 2)) goto fim;
    puts("[OK] Round Robin ignorou A1 e A4 depois do termino.");

    /* Verifica contagem de todos os seis processos terminados. */
    if (!terminar_ativo(&a, 2, 42, 3) ||
        !terminar_ativo(&a, 3, 43, 5) ||
        !terminar_ativo(&a, 5, 45, 6) ||
        !terminar_ativo(&a, 6, 46, 0)) goto fim;
    if (!enviar_irq_teste(&a, IRQ0) || kill(a.kernel, 0) != 0) goto fim;
    puts("[OK] Todas terminaram; o KernelSim permanece ativo.");
    ok = 1;

fim:
    encerrar_ambiente(&a);
    if (!ok) puts("[FALHOU] Registro e escalonamento dos terminados.");
    return ok;
}

int testar_termino(void) {
    puts("\n[TesteTermino] Verificando fim natural e retirada do escalonador...");
    int aplicacao = testar_aviso_da_aplicacao();
    int kernel = testar_registro_e_escalonamento();
    return aplicacao && kernel;
}
