#include "testes.h"
#include "../trabalho.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Teste deterministico do Round Robin.
 * Criamos seis processos auxiliares, controlados por SIGSTOP/SIGCONT,
 * e enviamos IRQ0 diretamente ao KernelSim. Assim, nao precisamos
 * esperar os sorteios ou os intervalos reais do InterController.
 */
static int fd_irq;
static int fd_pedidos;
static int fd_log;
static int fd_resposta[QUANTIDADE_APLICACOES];
static pid_t pids[QUANTIDADE_APLICACOES];
static char historico[16384];
static size_t tamanho_historico;
static size_t inicio_busca;

/* Le as mensagens do KernelSim ate encontrar o trecho solicitado. */
static int aguardar_mensagem(const char *mensagem) {
    for (int tentativa = 0; tentativa < 40; tentativa++) {
        char *trecho = strstr(historico + inicio_busca, mensagem);
        if (trecho) {
            inicio_busca = (size_t)(trecho - historico) + strlen(mensagem);
            return 1;
        }

        struct pollfd evento = { .fd = fd_log, .events = POLLIN };
        int retorno = poll(&evento, 1, 100);
        if (retorno < 0 && errno == EINTR) continue;
        if (retorno <= 0) continue;
        if (!(evento.revents & POLLIN) ||
            tamanho_historico + 1 >= sizeof historico) break;

        ssize_t lidos = read(fd_log, historico + tamanho_historico,
                             sizeof historico - tamanho_historico - 1);
        if (lidos <= 0) break;
        tamanho_historico += (size_t)lidos;
        historico[tamanho_historico] = '\0';
    }
    fprintf(stderr, "[TesteEscalonamento] Nao apareceu: %s\n", mensagem);
    fprintf(stderr, "[TesteEscalonamento] Ultimas mensagens: %.350s\n",
            historico + (tamanho_historico > 350 ? tamanho_historico - 350 : 0));
    return 0;
}

/* No Linux, o campo State de /proc/PID/status indica 'T' se parado. */
static char estado_linux(pid_t pid) {
    char caminho[64], linha[128], estado = '?';
    snprintf(caminho, sizeof caminho, "/proc/%ld/status", (long)pid);
    FILE *arquivo = fopen(caminho, "r");
    if (!arquivo) return '?';
    while (fgets(linha, sizeof linha, arquivo)) {
        if (sscanf(linha, "State: %c", &estado) == 1) break;
    }
    fclose(arquivo);
    return estado;
}

/* Confirma que SOMENTE a aplicacao esperada recebeu SIGCONT. */
static int conferir_execucao(int esperado) {
    for (int tentativa = 0; tentativa < 100; tentativa++) {
        int ativos = 0, id_ativo = 0, invalido = 0;
        for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
            char estado = estado_linux(pids[i]);
            if (estado == 'R' || estado == 'S' || estado == 'D') {
                ativos++;
                id_ativo = i + 1;
            } else if (estado != 'T' && estado != 't') {
                invalido = 1;
            }
        }
        if (!invalido && ativos == 1 && id_ativo == esperado) return 1;
        usleep(10000); /* Espera os sinais serem efetivamente aplicados. */
    }
    fprintf(stderr,
            "[TesteEscalonamento] Esperava somente A%d executavel.\n",
            esperado);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        fprintf(stderr, "  A%d: estado Linux = %c\n",
                i + 1, estado_linux(pids[i]));
    return 0;
}

/* Envia uma interrupcao de tempo e confere o proximo escalonado. */
static int avancar_para(int esperado) {
    MensagemIRQ mensagem = { .tipo = IRQ0 };
    char texto[64];
    snprintf(texto, sizeof texto, "[Kernel] Executando A%d\n", esperado);
    if (write(fd_irq, &mensagem, sizeof mensagem) !=
        (ssize_t)sizeof mensagem) return 0;
    return aguardar_mensagem(texto) && conferir_execucao(esperado);
}

/* Fecha um descritor que tenha sido criado. */
static void fechar(int *fd) {
    if (*fd >= 0) close(*fd);
    *fd = -1;
}

int testar_escalonamento(void) {
    int canal_irq[2] = {-1, -1};
    int canal_pedidos[2] = {-1, -1};
    int canal_log[2] = {-1, -1};
    int canais_resposta[QUANTIDADE_APLICACOES][2];
    pid_t kernel = -1;
    int criados = 0, passou = 0;
    struct sigaction ignorar = {0}, sigpipe_anterior;

    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        canais_resposta[i][0] = -1;
        canais_resposta[i][1] = -1;
        pids[i] = -1;
    }
    tamanho_historico = inicio_busca = 0;
    historico[0] = '\0';
    puts("\n[TesteEscalonamento] Verificando Round Robin e sinais...");

    /* Evita que um KernelSim encerrado mate o programa de testes por SIGPIPE. */
    ignorar.sa_handler = SIG_IGN;
    sigemptyset(&ignorar.sa_mask);
    if (sigaction(SIGPIPE, &ignorar, &sigpipe_anterior) == -1) return 0;
    if (access("./KernelSim", X_OK) != 0) {
        puts("[ERRO] Compile KernelSim antes de executar os testes.");
        goto limpar;
    }
    if (pipe(canal_irq) || pipe(canal_pedidos) || pipe(canal_log)) goto limpar;
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++)
        if (pipe(canais_resposta[i])) goto limpar;

    /* Processos auxiliares nao executam instrucoes de Application. */
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        pid_t filho = fork();
        if (filho == -1) goto limpar;
        if (filho == 0) {
            fechar(&canal_irq[0]); fechar(&canal_irq[1]);
            fechar(&canal_pedidos[0]); fechar(&canal_pedidos[1]);
            fechar(&canal_log[0]); fechar(&canal_log[1]);
            for (int j = 0; j < QUANTIDADE_APLICACOES; j++) {
                fechar(&canais_resposta[j][0]);
                fechar(&canais_resposta[j][1]);
            }
            raise(SIGSTOP);
            for (;;) pause();
        }
        pids[i] = filho;
        criados++;
        int estado;
        if (waitpid(filho, &estado, WUNTRACED) != filho ||
            !WIFSTOPPED(estado)) goto limpar;
    }

    kernel = fork();
    if (kernel == -1) goto limpar;
    if (kernel == 0) {
        fechar(&canal_irq[1]);
        fechar(&canal_pedidos[1]);
        fechar(&canal_log[0]);
        dup2(canal_log[1], STDOUT_FILENO);
        dup2(canal_log[1], STDERR_FILENO);
        fechar(&canal_log[1]);

        char textos[QUANTIDADE_APLICACOES * 2 + 2][32];
        char *argumentos[QUANTIDADE_APLICACOES * 2 + 4];
        argumentos[0] = "./KernelSim";
        snprintf(textos[0], sizeof textos[0], "%d", canal_irq[0]);
        snprintf(textos[1], sizeof textos[1], "%d", canal_pedidos[0]);
        argumentos[1] = textos[0];
        argumentos[2] = textos[1];
        for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
            fechar(&canais_resposta[i][0]);
            snprintf(textos[2 + i], sizeof textos[2 + i], "%d",
                     canais_resposta[i][1]);
            snprintf(textos[2 + QUANTIDADE_APLICACOES + i],
                     sizeof textos[2 + QUANTIDADE_APLICACOES + i],
                     "%ld", (long)pids[i]);
            argumentos[3 + i] = textos[2 + i];
            argumentos[3 + QUANTIDADE_APLICACOES + i] =
                textos[2 + QUANTIDADE_APLICACOES + i];
        }
        argumentos[3 + QUANTIDADE_APLICACOES * 2] = NULL;
        execv("./KernelSim", argumentos);
        _exit(127);
    }

    fechar(&canal_irq[0]);
    fechar(&canal_pedidos[0]);
    fechar(&canal_log[1]);
    fd_irq = canal_irq[1];
    fd_pedidos = canal_pedidos[1];
    fd_log = canal_log[0];
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        fechar(&canais_resposta[i][1]);
        fd_resposta[i] = canais_resposta[i][0];
    }

    /* 1: Inicio em A1, passagem por todos e retorno a A1. */
    if (!aguardar_mensagem("[Kernel] Executando A1\n") ||
        !conferir_execucao(1)) goto limpar;
    for (int i = 2; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar_para(i)) goto limpar;
    if (!avancar_para(1)) goto limpar;
    puts("[OK] IRQ0 executou A1, A2, A3, A4, A5, A6, A1.");
    puts("[OK] Apenas uma aplicacao recebeu SIGCONT de cada vez.");

    /* 2: A1 solicita RECV, para de executar e da lugar a A2. */
    PedidoSyscall pedido = {
        .id_aplicacao = 1, .operacao = RECEBER, .pc = 23, .n = 17
    };
    if (write(fd_pedidos, &pedido, sizeof pedido) !=
        (ssize_t)sizeof pedido ||
        !aguardar_mensagem("[Kernel] Bloqueou A1 (RECV, PC=23") ||
        !aguardar_mensagem("[Kernel] Executando A2\n") ||
        !conferir_execucao(2)) goto limpar;
    puts("[OK] A1 bloqueada por RECV; A2 escalonada imediatamente.");

    /* 3: Durante uma volta completa, A1 bloqueada e ignorada. */
    for (int i = 3; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar_para(i)) goto limpar;
    if (!avancar_para(2)) goto limpar;
    puts("[OK] Round Robin ignorou a aplicacao bloqueada.");

    /* 4: IRQ1 libera A1, mas ela nao interrompe A2 imediatamente. */
    MensagemIRQ interrupcao = { .tipo = IRQ1 };
    if (write(fd_irq, &interrupcao, sizeof interrupcao) !=
        (ssize_t)sizeof interrupcao ||
        !aguardar_mensagem("[Kernel] Concluiu RECV de A1; A1 agora PRONTO.") ||
        !conferir_execucao(2)) goto limpar;
    struct pollfd evento = { .fd = fd_resposta[0], .events = POLLIN };
    if (poll(&evento, 1, 1000) <= 0 || !(evento.revents & POLLIN)) goto limpar;
    RespostaSyscall resposta;
    if (read(fd_resposta[0], &resposta, sizeof resposta) !=
        (ssize_t)sizeof resposta || resposta.id_aplicacao != 1 ||
        resposta.operacao != RECEBER || resposta.n != 0) goto limpar;

    for (int i = 3; i <= QUANTIDADE_APLICACOES; i++)
        if (!avancar_para(i)) goto limpar;
    if (!avancar_para(1)) goto limpar;
    puts("[OK] A1 desbloqueada voltou a fila e esperou sua vez.");

    /* 5: Interrupcoes sem processos bloqueados nao trocam a CPU. */
    interrupcao.tipo = IRQ1;
    if (write(fd_irq, &interrupcao, sizeof interrupcao) !=
        (ssize_t)sizeof interrupcao ||
        !aguardar_mensagem("[Kernel] IRQ1 ignorada: fila vazia.") ||
        !conferir_execucao(1)) goto limpar;
    interrupcao.tipo = IRQ2;
    if (write(fd_irq, &interrupcao, sizeof interrupcao) !=
        (ssize_t)sizeof interrupcao ||
        !aguardar_mensagem("[Kernel] IRQ2 ignorada: fila vazia.") ||
        !conferir_execucao(1)) goto limpar;
    puts("[OK] IRQ1 e IRQ2 sem pendencias nao alteraram o escalonamento.");
    passou = 1;

limpar:
    if (kernel > 0) {
        kill(kernel, SIGTERM);
        waitpid(kernel, NULL, 0);
    }
    for (int i = 0; i < criados; i++) {
        kill(pids[i], SIGCONT);
        kill(pids[i], SIGTERM);
        waitpid(pids[i], NULL, 0);
    }
    fechar(&canal_irq[0]); fechar(&canal_irq[1]);
    fechar(&canal_pedidos[0]); fechar(&canal_pedidos[1]);
    fechar(&canal_log[0]); fechar(&canal_log[1]);
    for (int i = 0; i < QUANTIDADE_APLICACOES; i++) {
        fechar(&canais_resposta[i][0]);
        fechar(&canais_resposta[i][1]);
    }
    sigaction(SIGPIPE, &sigpipe_anterior, NULL);
    if (!passou) puts("[FALHOU] Teste do escalonamento Round Robin.");
    return passou;
}
