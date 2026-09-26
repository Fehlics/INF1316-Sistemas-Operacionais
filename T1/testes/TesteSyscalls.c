#include "testes.h"
#include "../trabalho.h"
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Verifica que Application espera a resposta de sua syscall. */
int testar_respostas(void) {
    int pedidos[2], respostas[2], saida[2];
    if (pipe(pedidos) || pipe(respostas) || pipe(saida)) return 0;
    pid_t filho = fork();
    if (filho < 0) return 0;
    if (filho == 0) {
        close(pedidos[0]); close(respostas[1]); close(saida[0]);
        dup2(saida[1], STDOUT_FILENO);
        close(saida[1]);
        char fd_pedido[16], fd_resposta[16];
        snprintf(fd_pedido, sizeof fd_pedido, "%d", pedidos[1]);
        snprintf(fd_resposta, sizeof fd_resposta, "%d", respostas[0]);
        setenv("TESTE_SYSCALL", "1", 1);
        execl("./Application", "Application", "1", fd_pedido, fd_resposta,
              (char *)NULL);
        _exit(127);
    }
    close(pedidos[1]); close(respostas[0]); close(saida[1]);
    int ok = 0;
    struct pollfd evento = { .fd = pedidos[0], .events = POLLIN };
    if (poll(&evento, 1, 4500) > 0 && (evento.revents & POLLIN)) {
        PedidoSyscall pedido;
        if (read(pedidos[0], &pedido, sizeof pedido) == (ssize_t)sizeof pedido &&
            pedido.id_aplicacao == 1 && pedido.operacao == ENVIAR && pedido.pc == 2) {
            int status;
            usleep(200000);
            if (waitpid(filho, &status, WNOHANG) == 0) {
                RespostaSyscall resposta = {
                    .id_aplicacao = 1, .operacao = ENVIAR, .n = 0
                };
                if (write(respostas[1], &resposta, sizeof resposta) ==
                    (ssize_t)sizeof resposta) {
                    char texto[2048] = {0};
                    size_t total = 0;
                    for (int i = 0; i < 20 && total < sizeof texto - 1; i++) {
                        struct pollfd p = { .fd = saida[0], .events = POLLIN };
                        if (poll(&p, 1, 100) <= 0) continue;
                        ssize_t n = read(saida[0], texto + total,
                                         sizeof texto - total - 1);
                        if (n <= 0) break;
                        total += (size_t)n;
                        texto[total] = 0;
                        if (strstr(texto, "SEND concluido (PC=2, N=0)")) {
                            ok = 1;
                            break;
                        }
                    }
                }
            }
        }
    }
    kill(filho, SIGTERM);
    waitpid(filho, NULL, 0);
    close(pedidos[0]); close(respostas[1]); close(saida[0]);
    puts(ok ? "[OK] Application esperou a resposta do SEND." :
              "[ERRO] Application nao aguardou a resposta.");
    return ok;
}
