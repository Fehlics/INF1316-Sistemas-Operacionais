#include "testes.h"
#include "apoio.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* Le uma estrutura inteira de uma pipe, mesmo quando read retorna
 * menos bytes. Apenas recursos presentes nos laboratorios da turma. */
static int ler_completo(int fd, void *destino, size_t tamanho) {
    size_t recebidos = 0;
    while (recebidos < tamanho) {
        ssize_t n = read(fd, (char *)destino + recebidos, tamanho - recebidos);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        recebidos += (size_t)n;
    }
    return 1;
}

/* Exercita a Application REAL, em vez de apenas inspecionar o PCB:
 * SIGSTOP/SIGCONT nao podem reiniciar seu PC nem apagar N. A resposta
 * da syscall restaura PC e N enviados pelo kernel simulado. */
static int verificar_aplicacao_real(void) {
    int controle[2], respostas[2];
    if (pipe(controle) != 0) return 0;
    if (pipe(respostas) != 0) {
        close(controle[0]); close(controle[1]);
        return 0;
    }
    pid_t filho = fork();
    if (filho < 0) {
        close(controle[0]); close(controle[1]);
        close(respostas[0]); close(respostas[1]);
        return 0;
    }
    if (filho == 0) {
        close(controle[0]);
        close(respostas[1]);
        FILE *saida = fopen("/dev/null", "w");
        if (saida) {
            dup2(fileno(saida), STDOUT_FILENO);
            fclose(saida);
        }
        char texto_controle[16], texto_resposta[16];
        snprintf(texto_controle, sizeof texto_controle, "%d", controle[1]);
        snprintf(texto_resposta, sizeof texto_resposta, "%d", respostas[0]);
        setenv("TESTE_MAX_ITERACOES", "3", 1);
        setenv("TESTE_SYSCALL", "1", 1); /* A2 faz RECV ao atingir PC=2. */
        execl("./Application", "Application", "2", texto_controle,
              texto_resposta, (char *)NULL);
        _exit(127);
    }
    close(controle[1]);
    close(respostas[0]);
    MensagemControle m;
    int status, ok = 0;

    /* A2 avisa que comecou PC=1; interrompemos durante o sono. */
    if (!ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_CONTEXTO || m.pedido.pc != 1 || m.pedido.n != 0 ||
        kill(filho, SIGSTOP) < 0 ||
        waitpid(filho, &status, WUNTRACED) != filho ||
        !WIFSTOPPED(status) || kill(filho, SIGCONT) < 0) goto fim;

    /* A mesma aplicacao prossegue em PC=2, nao volta ao PC inicial. */
    if (!ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_CONTEXTO || m.pedido.pc != 2 || m.pedido.n != 0 ||
        !ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_SYSCALL || m.pedido.id_aplicacao != 2 ||
        m.pedido.operacao != RECEBER || m.pedido.endereco != ENDERECO_N ||
        m.pedido.pc != 2 || m.pedido.n != 0) goto fim;

    /* Emula o kernel que registrou PC=2, fez a leitura e agora devolve
     * todo o contexto simulado, inclusive o novo N=77. */
    if (kill(filho, SIGSTOP) < 0 ||
        waitpid(filho, &status, WUNTRACED) != filho || !WIFSTOPPED(status))
        goto fim;
    RespostaSyscall retorno = {2, RECEBER, 77, 2};
    if (write(respostas[1], &retorno, sizeof retorno) !=
        (ssize_t)sizeof retorno || kill(filho, SIGCONT) < 0) goto fim;

    /* A resposta restabelece PC/N, que permanecem corretos na proxima
     * iteracao mesmo apos os dois ciclos de parada/retomada. */
    if (!ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_CONTEXTO || m.pedido.pc != 2 || m.pedido.n != 77 ||
        !ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_CONTEXTO || m.pedido.pc != 3 || m.pedido.n != 77 ||
        !ler_completo(controle[0], &m, sizeof m) ||
        m.tipo != EVENTO_TERMINO || m.pedido.pc != 3 || m.pedido.n != 77)
        goto fim;
    if (waitpid(filho, &status, WNOHANG) != 0) goto fim;
    retorno.operacao = NENHUMA_OPERACAO;
    retorno.pc = 3;
    if (write(respostas[1], &retorno, sizeof retorno) !=
        (ssize_t)sizeof retorno) goto fim;
    for (int tentativa = 0; tentativa < 100; tentativa++) {
        pid_t r = waitpid(filho, &status, WNOHANG);
        if (r == filho) {
            filho = -1;
            ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            break;
        }
        if (r < 0) break;
        usleep(10000);
    }

fim:
    if (filho > 0) {
        kill(filho, SIGCONT);
        kill(filho, SIGTERM);
        waitpid(filho, NULL, 0);
    }
    close(controle[0]);
    close(respostas[1]);
    if (ok) puts("[OK] PC e N preservados por SIGSTOP/SIGCONT e resposta RECV.");
    else puts("[ERRO] Contexto da Application nao preservado apos retomada.");
    return ok;
}

/* Confere o endereco logico da syscall no PCB e sua limpeza apos IRQ2. */
static int verificar_parametros_no_pcb(void) {
    AmbienteTeste a;
    int ok = 0;
    if (!iniciar_ambiente(&a)) return 0;
    if (!enviar_pedido_teste(&a, 1, ENVIAR, 21, 8) ||
        !esperar_ativo(&a, 2)) goto fim;
    MensagemControle evento = {0};
    evento.tipo = EVENTO_PAUSAR;
    if (write(a.controle, &evento, sizeof evento) != (ssize_t)sizeof evento)
        goto fim;
    EstadoSimulador estado;
    if (!ler_completo(a.estados, &estado, sizeof estado) || estado.fase != 1 ||
        !verificar_parada(a.auxiliares[1])) goto fim;
    evento.tipo = EVENTO_MOSTRAR;
    if (write(a.controle, &evento, sizeof evento) != (ssize_t)sizeof evento ||
        !ler_completo(a.estados, &estado, sizeof estado) || estado.fase != 2 ||
        estado.processos[0].estado != BLOQUEADO_ESCRITA ||
        estado.processos[0].operacao_pendente != ENVIAR ||
        estado.processos[0].endereco_pendente != ENDERECO_PC ||
        estado.processos[0].pc != 21 || estado.processos[0].n != 8) goto fim;

    evento.tipo = EVENTO_RETOMAR;
    if (write(a.controle, &evento, sizeof evento) != (ssize_t)sizeof evento ||
        !ler_completo(a.estados, &estado, sizeof estado) || estado.fase != 3 ||
        !esperar_ativo(&a, 2) || !enviar_irq_teste(&a, IRQ2) ||
        !ler_resposta_teste(&a, 1, ENVIAR, 8)) goto fim;

    evento.tipo = EVENTO_PAUSAR;
    if (write(a.controle, &evento, sizeof evento) != (ssize_t)sizeof evento ||
        !ler_completo(a.estados, &estado, sizeof estado) || estado.fase != 1 ||
        !verificar_parada(a.auxiliares[1])) goto fim;
    evento.tipo = EVENTO_MOSTRAR;
    if (write(a.controle, &evento, sizeof evento) != (ssize_t)sizeof evento ||
        !ler_completo(a.estados, &estado, sizeof estado) || estado.fase != 2 ||
        estado.processos[0].estado != PRONTO ||
        estado.processos[0].operacao_pendente != NENHUMA_OPERACAO ||
        estado.processos[0].endereco_pendente != SEM_ENDERECO ||
        estado.processos[0].pc != 21 || estado.processos[0].n != 8) goto fim;
    ok = 1;

fim:
    encerrar_ambiente(&a);
    if (ok) puts("[OK] PCB guardou SEND/&PC e limpou parametros apos IRQ2.");
    else puts("[ERRO] Parametros de syscall nao preservados no PCB.");
    return ok;
}

int testar_contexto(void) {
    puts("\n[TesteContexto] Verificando salvamento e retomada do contexto...");
    int aplicacao = verificar_aplicacao_real();
    int pcb = verificar_parametros_no_pcb();
    return aplicacao && pcb;
}
