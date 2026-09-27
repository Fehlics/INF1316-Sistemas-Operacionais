#include "testes.h"
#include "apoio.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Le a estrutura completa produzida pelo KernelSim sem poll/select. */
static int receber_fotografia(AmbienteTeste *a, EstadoSimulador *estado,
                             int fase) {
    size_t total = 0;
    while (total < sizeof *estado) {
        ssize_t n = read(a->estados, (char *)estado + total,
                         sizeof *estado - total);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        total += (size_t)n;
    }
    return estado->fase == fase;
}

static int evento_simulador(AmbienteTeste *a, TipoEvento tipo) {
    MensagemControle m = {0};
    m.tipo = tipo;
    return write(a->controle, &m, sizeof m) == (ssize_t)sizeof m;
}

static int atualizar_pc_e_n(AmbienteTeste *a, int id, int pc, int n) {
    MensagemControle m = {0};
    m.tipo = EVENTO_CONTEXTO;
    m.pedido.id_aplicacao = id;
    m.pedido.pc = pc;
    m.pedido.n = n;
    return write(a->controle, &m, sizeof m) == (ssize_t)sizeof m;
}

/* Injeta IRQs de forma controlada e confere contexto/pausa/retomada. */
static int testar_kernel_pausado(void) {
    AmbienteTeste a;
    EstadoSimulador e;
    int ok = 0;
    if (!iniciar_ambiente(&a)) return 0;

    /* Antes de IRQ0, A1 atualiza seu contexto sem executar syscall. */
    if (!atualizar_pc_e_n(&a, 1, 7, 55) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 2) ||
        !atualizar_pc_e_n(&a, 2, 9, 66) ||
        !evento_simulador(&a, EVENTO_PAUSAR) ||
        !receber_fotografia(&a, &e, 1) || e.executando != 2 ||
        !verificar_parada(a.auxiliares[1]) ||
        !evento_simulador(&a, EVENTO_MOSTRAR) ||
        !receber_fotografia(&a, &e, 2)) goto fim;
    if (e.processos[0].pc != 7 || e.processos[0].n != 55 ||
        e.processos[0].estado != PRONTO ||
        e.processos[1].pc != 9 || e.processos[1].n != 66 ||
        e.processos[1].estado != EXECUTANDO) goto fim;
    puts("[OK] Fotografia incluiu PC/N atualizados sem syscalls.");

    /* IRQ0 recebido durante a pausa nao altera o escalonador. */
    if (!enviar_irq_teste(&a, IRQ0) ||
        !evento_simulador(&a, EVENTO_MOSTRAR) ||
        !receber_fotografia(&a, &e, 2) ||
        e.executando != 2 || e.processos[1].estado != EXECUTANDO ||
        e.processos[2].estado != PRONTO ||
        !evento_simulador(&a, EVENTO_RETOMAR) ||
        !receber_fotografia(&a, &e, 3) ||
        !esperar_ativo(&a, 2) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 3)) goto fim;
    puts("[OK] IRQ0 nao alterou a CPU pausada; retomada preservou a ordem.");

    /* Os parametros de syscall devem ser conservados no PCB. */
    if (!enviar_pedido_teste(&a, 3, RECEBER, 23, 17) ||
        !esperar_ativo(&a, 4) ||
        !evento_simulador(&a, EVENTO_PAUSAR) ||
        !receber_fotografia(&a, &e, 1) ||
        !verificar_parada(a.auxiliares[3]) ||
        !evento_simulador(&a, EVENTO_MOSTRAR) ||
        !receber_fotografia(&a, &e, 2)) goto fim;
    if (e.processos[2].estado != BLOQUEADO_LEITURA ||
        e.processos[2].operacao_pendente != RECEBER ||
        e.processos[2].pc != 23 || e.processos[2].n != 17 ||
        e.processos[3].estado != EXECUTANDO) goto fim;
    puts("[OK] Fotografia registrou processo bloqueado e parametros da syscall.");

    /* Mesmo uma IRQ injetada artificialmente nao deve desbloquear A3
       enquanto a simulacao inteira estiver pausada. */
    if (!enviar_irq_teste(&a, IRQ1) ||
        !evento_simulador(&a, EVENTO_MOSTRAR) ||
        !receber_fotografia(&a, &e, 2) ||
        e.processos[2].estado != BLOQUEADO_LEITURA ||
        e.processos[2].operacao_pendente != RECEBER) goto fim;
    puts("[OK] IRQ1 durante a pausa nao desbloqueou a aplicacao.");

    if (!evento_simulador(&a, EVENTO_RETOMAR) ||
        !receber_fotografia(&a, &e, 3) || !esperar_ativo(&a, 4) ||
        !enviar_irq_teste(&a, IRQ1) ||
        !ler_resposta_teste(&a, 3, RECEBER, 0) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 5) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 6) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 1) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 2) ||
        !enviar_irq_teste(&a, IRQ0) || !esperar_ativo(&a, 3)) goto fim;
    puts("[OK] Processo desbloqueado retomou pelo Round Robin apos a pausa.");
    ok = 1;

fim:
    encerrar_ambiente(&a);
    if (!ok) puts("[ERRO] Verificacao deterministica da pausa e do contexto.");
    return ok;
}

/* Testa tambem os SIGTSTP reais recebidos pelo Simulador.
 * Esperamos 2s para a criacao, pausamos, retomamos e encerramos. */
static int testar_controle_real(void) {
    int registro[2];
    if (pipe(registro) < 0) return 0;
    pid_t simulador = fork();
    if (simulador < 0) {
        close(registro[0]); close(registro[1]);
        return 0;
    }
    if (simulador == 0) {
        close(registro[0]);
        dup2(registro[1], STDOUT_FILENO);
        close(registro[1]);
        setenv("TESTE_MAX_ITERACOES", "3", 1);
        setenv("TESTE_SEM_SYSCALL", "1", 1);
        execl("./Simulador", "Simulador", (char *)NULL);
        _exit(127);
    }
    close(registro[1]);
    sleep(2);
    kill(simulador, SIGTSTP);
    sleep(2);
    kill(simulador, SIGTSTP);
    sleep(2);
    kill(simulador, SIGINT);
    int situacao = 0, recolhido = 0;
    for (int t = 0; t < 60; t++) {
        pid_t n = waitpid(simulador, &situacao, WNOHANG);
        if (n == simulador) { recolhido = 1; break; }
        if (n < 0) break;
        usleep(100000);
    }
    if (!recolhido) {
        kill(simulador, SIGTERM);
        waitpid(simulador, &situacao, 0);
    }
    char texto[16000] = {0};
    size_t total = 0;
    while (total + 1 < sizeof texto) {
        ssize_t n = read(registro[0], texto + total,
                         sizeof texto - total - 1);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        total += (size_t)n;
        texto[total] = 0;
    }
    close(registro[0]);
    int ok = recolhido && WIFEXITED(situacao) &&
             WEXITSTATUS(situacao) == 130 &&
             strstr(texto, "ESTADO DOS PROCESSOS (PAUSADO)") &&
             strstr(texto, "[Estado] A1") &&
             strstr(texto, "[Estado] A6") &&
             strstr(texto, "Execucao retomada.");
    if (ok) puts("[OK] SIGTSTP pausou e retomou o Simulador real.");
    else {
        puts("[ERRO] SIGTSTP nao pausou/retomou o Simulador real.");
        fprintf(stderr, "[TestePausa] Registro completo:\n%s\n", texto);
    }
    return ok;
}

int testar_pausa(void) {
    puts("\n[TestePausa] Verificando contexto e Ctrl+Z...");
    int kernel = testar_kernel_pausado();
    int controle = testar_controle_real();
    return kernel && controle;
}
