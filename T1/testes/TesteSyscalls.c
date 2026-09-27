#include "testes.h"
#include "../trabalho.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Executa uma aplicacao real, envia uma resposta e confere sua saida. */
static int testar_operacao(int id, Operacao operacao, int valor) {
    int controle[2], resposta[2], saida[2];
    if (pipe(controle) || pipe(resposta) || pipe(saida)) return 0;
    pid_t filho = fork();
    if (filho < 0) return 0;
    if (filho == 0) {
        close(controle[0]); close(resposta[1]); close(saida[0]);
        dup2(saida[1], STDOUT_FILENO);
        close(saida[1]);
        char id_texto[12], fd_controle[12], fd_resposta[12];
        snprintf(id_texto, sizeof id_texto, "%d", id);
        snprintf(fd_controle, sizeof fd_controle, "%d", controle[1]);
        snprintf(fd_resposta, sizeof fd_resposta, "%d", resposta[0]);
        /* O modo de teste gera a operacao desejada ao atingir PC=2. */
        setenv("TESTE_SYSCALL", "1", 1);
        execl("./Application", "Application", id_texto,
              fd_controle, fd_resposta, (char *)NULL);
        _exit(127);
    }
    close(controle[1]); close(resposta[0]); close(saida[1]);
    int ok = 0;
    MensagemControle mensagem;
    ssize_t lidos;
    int atualizacoes = 0;
    /* Application emite EVENTO_CONTEXTO antes de cada passo. */
    do {
        lidos = read(controle[0], &mensagem, sizeof mensagem);
        if (lidos == (ssize_t)sizeof mensagem &&
            mensagem.tipo == EVENTO_CONTEXTO) atualizacoes++;
    } while (lidos == (ssize_t)sizeof mensagem &&
             mensagem.tipo == EVENTO_CONTEXTO && atualizacoes < 3);
    if (atualizacoes == 2 && lidos == (ssize_t)sizeof mensagem &&
        mensagem.tipo == EVENTO_SYSCALL &&
        mensagem.pedido.id_aplicacao == id &&
        mensagem.pedido.operacao == operacao &&
        mensagem.pedido.endereco ==
            (operacao == ENVIAR ? ENDERECO_PC : ENDERECO_N) &&
        mensagem.pedido.pc == 2) {
        int estado;
        /* O filho continua vivo porque seu read da resposta bloqueia. */
        if (waitpid(filho, &estado, WNOHANG) == 0) {
            RespostaSyscall r = {id, operacao, valor, 2};
            if (write(resposta[1], &r, sizeof r) == (ssize_t)sizeof r) {
                char texto[1024] = {0}, esperado[100];
                size_t total = 0;
                snprintf(esperado, sizeof esperado,
                         "[A%d] %s concluido (PC=2, N=%d)", id,
                         operacao == ENVIAR ? "SEND" : "RECV",
                         operacao == ENVIAR ? 0 : valor);
                for (int i = 0; i < 10 && total < sizeof texto - 1; i++) {
                    ssize_t n = read(saida[0], texto + total,
                                     sizeof texto - total - 1);
                    if (n <= 0) break;
                    total += (size_t)n;
                    texto[total] = '\0';
                    if (strstr(texto, esperado)) { ok = 1; break; }
                }
            }
        }
    }
    kill(filho, SIGTERM);
    waitpid(filho, NULL, 0);
    close(controle[0]); close(resposta[1]); close(saida[0]);
    return ok;
}

int testar_respostas(void) {
    puts("\n[TesteSyscalls] Testando respostas de SEND e RECV...");
    int envio = testar_operacao(1, ENVIAR, 0);
    int leitura = testar_operacao(2, RECEBER, 1234);
    if (envio && leitura) puts("[OK] Aplicacoes aguardaram as respostas.");
    return envio && leitura;
}
