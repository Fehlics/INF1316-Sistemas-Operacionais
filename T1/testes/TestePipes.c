#include "testes.h"
#include "../trabalho.h"

#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Testa os pipes simulados sem depender de sorteios.
 * Seis processos ficticios representam A1...A6. O teste envia syscalls
 * reais ao KernelSim e gera IRQs de forma controlada.
 */
static int irq_fd, pedidos_fd, log_fd;
static int resposta_fd[QUANTIDADE_APLICACOES];
static char historico[32768];
static size_t lidos, posicao;

/* Espera o Kernel reconhecer um pedido antes de gerar a interrupcao. */
static int esperar_log(const char *texto) {
    for (int tentativa = 0; tentativa < 30; tentativa++) {
        char *encontrado = strstr(historico + posicao, texto);
        if (encontrado) {
            posicao = (size_t)(encontrado - historico) + strlen(texto);
            return 1;
        }
        struct pollfd fd = { .fd = log_fd, .events = POLLIN };
        if (poll(&fd, 1, 100) <= 0) continue;
        if (!(fd.revents & POLLIN) || lidos + 1 >= sizeof historico) return 0;
        ssize_t n = read(log_fd, historico + lidos, sizeof historico - lidos - 1);
        if (n <= 0) return 0;
        lidos += (size_t)n;
        historico[lidos] = '\0';
    }
    fprintf(stderr, "[TestePipes] Timeout: %s\n", texto);
    return 0;
}

static int enviar_pedido(int id, Operacao op, int pc) {
    PedidoSyscall pedido = {
        .id_aplicacao = id, .operacao = op, .pc = pc, .n = id * 10
    };
    char texto[80];
    snprintf(texto, sizeof texto, "[Kernel] Bloqueou A%d (%s, PC=%d",
             id, op == ENVIAR ? "SEND" : "RECV", pc);
    return write(pedidos_fd, &pedido, sizeof pedido) == (ssize_t)sizeof pedido &&
           esperar_log(texto);
}

static int gerar_irq(TipoIRQ tipo) {
    MensagemIRQ mensagem = { .tipo = tipo };
    return write(irq_fd, &mensagem, sizeof mensagem) ==
           (ssize_t)sizeof mensagem;
}

static int conferir_resposta(int id, Operacao op, int esperado) {
    struct pollfd fd = { .fd = resposta_fd[id - 1], .events = POLLIN };
    if (poll(&fd, 1, 1500) <= 0 || !(fd.revents & POLLIN)) return 0;
    RespostaSyscall resposta;
    if (read(resposta_fd[id - 1], &resposta, sizeof resposta) !=
        (ssize_t)sizeof resposta) return 0;
    if (resposta.id_aplicacao != id || resposta.operacao != op ||
        resposta.n != esperado) {
        fprintf(stderr, "[TestePipes] A%d: N esperado=%d, recebido=%d\n",
                id, esperado, resposta.n);
        return 0;
    }
    return 1;
}

static int sem_resposta(int id) {
    struct pollfd fd = { .fd = resposta_fd[id - 1], .events = POLLIN };
    return poll(&fd, 1, 50) == 0;
}

/* Solicita e conclui uma escrita por IRQ2. */
static int escrever(int id, int pc) {
    return enviar_pedido(id, ENVIAR, pc) && gerar_irq(IRQ2) &&
           conferir_resposta(id, ENVIAR, id * 10);
}

/* Solicita e conclui uma leitura por IRQ1. */
static int ler(int id, int esperado) {
    return enviar_pedido(id, RECEBER, 100 + id) && gerar_irq(IRQ1) &&
           conferir_resposta(id, RECEBER, esperado);
}

int testar_pipes(void) {
    int irq[2] = {-1, -1}, pedidos[2] = {-1, -1}, log[2] = {-1, -1};
    int respostas[QUANTIDADE_APLICACOES][2];
    pid_t filhos[QUANTIDADE_APLICACOES] = {0}, kernel = -1;
    int criados = 0, passou = 0;
    memset(respostas, -1, sizeof respostas);
    memset(historico, 0, sizeof historico);
    lidos = posicao = 0;

    puts("\n[TestePipes] Testando os seis buffers bidirecionais...");
    if (pipe(irq) || pipe(pedidos) || pipe(log)) goto limpar;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (pipe(respostas[i])) goto limpar;

    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pid_t filho = fork();
        if (filho == -1) goto limpar;
        if (filho == 0) {
            /* Dummy: apenas recebe SIGSTOP / SIGCONT do KernelSim. */
            raise(SIGSTOP);
            for (;;) pause();
        }
        filhos[i] = filho;
        criados++;
        int estado;
        if (waitpid(filho, &estado, WUNTRACED) != filho ||
            !WIFSTOPPED(estado)) goto limpar;
    }

    kernel = fork();
    if (kernel == -1) goto limpar;
    if (kernel == 0) {
        close(irq[1]); close(pedidos[1]); close(log[0]);
        dup2(log[1], STDOUT_FILENO);
        dup2(log[1], STDERR_FILENO);
        close(log[1]);
        char textos[QUANTIDADE_APLICACOES * 2 + 2][32];
        char *args[QUANTIDADE_APLICACOES * 2 + 4];
        args[0] = "./KernelSim";
        snprintf(textos[0], sizeof textos[0], "%d", irq[0]);
        snprintf(textos[1], sizeof textos[1], "%d", pedidos[0]);
        args[1] = textos[0]; args[2] = textos[1];
        for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
            close(respostas[i][0]);
            snprintf(textos[i + 2], sizeof textos[i + 2], "%d", respostas[i][1]);
            snprintf(textos[i + QUANTIDADE_APLICACOES + 2],
                     sizeof textos[i + QUANTIDADE_APLICACOES + 2], "%ld",
                     (long)filhos[i]);
            args[3 + i] = textos[2 + i];
            args[3 + QUANTIDADE_APLICACOES + i] =
                textos[2 + QUANTIDADE_APLICACOES + i];
        }
        args[3 + QUANTIDADE_APLICACOES * 2] = NULL;
        execv("./KernelSim", args);
        _exit(127);
    }

    close(irq[0]); irq[0] = -1;
    close(pedidos[0]); pedidos[0] = -1;
    close(log[1]); log[1] = -1;
    irq_fd = irq[1]; pedidos_fd = pedidos[1]; log_fd = log[0];
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        close(respostas[i][1]); respostas[i][1] = -1;
        resposta_fd[i] = respostas[i][0];
    }
    if (!esperar_log("[Kernel] Executando A1")) goto limpar;

    /* Uma leitura sem mensagem nao fica esperando pelo parceiro. */
    if (!ler(2, 0)) goto limpar;
    puts("[OK] RECV em buffer vazio devolveu N=0.");

    /* Os buffers sao indexados pelo remetente, nao pelo destinatario. */
    if (!escrever(1, 11) || !escrever(1, 12) ||
        !escrever(3, 31) || !escrever(5, 51) ||
        !escrever(2, 21)) goto limpar;
    puts("[OK] SEND gravou mensagens dos pares e dos dois sentidos.");

    /* IRQ1 respeita a fila GLOBAL de leituras, mesmo em pipes diferentes. */
    if (!enviar_pedido(4, RECEBER, 104) ||
        !enviar_pedido(6, RECEBER, 106) ||
        !gerar_irq(IRQ1) || !conferir_resposta(4, RECEBER, 31) ||
        !sem_resposta(6) ||
        !gerar_irq(IRQ1) || !conferir_resposta(6, RECEBER, 51))
        goto limpar;
    puts("[OK] IRQ1 concluiu leituras FIFO em pipes independentes.");

    if (!ler(2, 11) || !ler(2, 12) || !ler(1, 21) ||
        !ler(2, 0) || !ler(4, 0) || !ler(6, 0)) goto limpar;
    puts("[OK] Mensagens FIFO, ambos os sentidos e isolamento dos pares.");

    /* O dado de SEND somente fica visivel APOS a IRQ2. */
    if (!enviar_pedido(1, ENVIAR, 55) || !sem_resposta(1) ||
        !ler(2, 0) || !gerar_irq(IRQ2) ||
        !conferir_resposta(1, ENVIAR, 10) || !ler(2, 55))
        goto limpar;
    puts("[OK] Escrita so ficou disponivel depois da IRQ2.");
    passou = 1;

limpar:
    if (kernel > 0) { kill(kernel, SIGTERM); waitpid(kernel, NULL, 0); }
    for (int i = 0; i < criados; i++) {
        kill(filhos[i], SIGCONT);
        kill(filhos[i], SIGTERM);
        waitpid(filhos[i], NULL, 0);
    }
    if (irq[0] >= 0) close(irq[0]);
    if (irq[1] >= 0) close(irq[1]);
    if (pedidos[0] >= 0) close(pedidos[0]);
    if (pedidos[1] >= 0) close(pedidos[1]);
    if (log[0] >= 0) close(log[0]);
    if (log[1] >= 0) close(log[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        if (respostas[i][0] >= 0) close(respostas[i][0]);
        if (respostas[i][1] >= 0) close(respostas[i][1]);
    }
    if (!passou) puts("[FALHOU] Teste dos buffers simulados.");
    return passou;
}
