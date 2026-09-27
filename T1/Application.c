#include "trabalho.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Envia o pedido pelo canal de controle e aguarda a resposta individual. */
static int solicitar_syscall(int fd_controle, int fd_resposta, int id,
                            Operacao operacao, int *pc, int *n) {
    MensagemControle mensagem = {0};
    mensagem.tipo = EVENTO_SYSCALL;
    mensagem.pedido.id_aplicacao = id;
    mensagem.pedido.operacao = operacao;
    mensagem.pedido.endereco = operacao == ENVIAR ? ENDERECO_PC : ENDERECO_N;
    mensagem.pedido.pc = *pc;
    mensagem.pedido.n = *n;

    ssize_t escritos;
    do {
        escritos = write(fd_controle, &mensagem, sizeof mensagem);
    } while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof mensagem) {
        perror("Application: envio da syscall");
        return 0;
    }
    printf("[A%d] Solicitou %s (PC=%d, N=%d)\n", id,
           operacao == ENVIAR ? "SEND" : "RECV", *pc, *n);

    RespostaSyscall resposta;
    size_t total = 0;
    while (total < sizeof resposta) {
        ssize_t lidos = read(fd_resposta, (char *)&resposta + total,
                             sizeof resposta - total);
        if (lidos == 0) return 0;
        if (lidos < 0) {
            if (errno == EINTR) continue;
            perror("Application: resposta");
            return 0;
        }
        total += (size_t)lidos;
    }
    if (resposta.id_aplicacao != id || resposta.operacao != operacao) return 0;
    /* Na volta de uma syscall, a resposta contem o contexto do PCB.
     * Fora das syscalls, SIGSTOP/SIGCONT preservam as variaveis locais
     * atraves do proprio Linux (nao ha manipulacao de registradores). */
    *pc = resposta.pc;
    *n = resposta.n;
    printf("[A%d] %s concluido (PC=%d, N=%d)\n", id,
           operacao == ENVIAR ? "SEND" : "RECV", *pc, *n);
    return 1;
}

/* Atualiza a copia de PC e N guardada no KernelSim. A escrita da mensagem
 * pequena ocorre de uma vez, como nos outros eventos de controle. */
static int informar_contexto(int controle, int id, int pc, int n) {
    MensagemControle mensagem = {0};
    mensagem.tipo = EVENTO_CONTEXTO;
    mensagem.pedido.id_aplicacao = id;
    mensagem.pedido.pc = pc;
    mensagem.pedido.n = n;
    ssize_t escritos;
    do { escritos = write(controle, &mensagem, sizeof mensagem); }
    while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof mensagem) {
        perror("Application: atualizacao de contexto");
        return 0;
    }
    return 1;
}

/* Avisa o kernel quando a ultima iteracao foi executada.
 * Aguarda confirmacao para que o kernel registre o termino antes do exit. */
static int avisar_termino(int controle, int fd_resposta, int id, int pc, int n) {
    MensagemControle mensagem = {0};
    mensagem.tipo = EVENTO_TERMINO;
    mensagem.pedido.id_aplicacao = id;
    mensagem.pedido.operacao = NENHUMA_OPERACAO;
    mensagem.pedido.pc = pc;
    mensagem.pedido.n = n;

    ssize_t escritos;
    do { escritos = write(controle, &mensagem, sizeof mensagem); }
    while (escritos < 0 && errno == EINTR);
    if (escritos != (ssize_t)sizeof mensagem) {
        perror("Application: aviso de termino");
        return 0;
    }

    /* Se for interrompida por SIGSTOP, o kernel enviara SIGCONT para
       permitir que a aplicacao receba a confirmacao e termine. */
    RespostaSyscall resposta;
    size_t total = 0;
    while (total < sizeof resposta) {
        ssize_t lidos = read(fd_resposta, (char *)&resposta + total,
                             sizeof resposta - total);
        if (lidos == 0) return 0;
        if (lidos < 0) {
            if (errno == EINTR) continue;
            perror("Application: confirmacao de termino");
            return 0;
        }
        total += (size_t)lidos;
    }
    return resposta.id_aplicacao == id &&
           resposta.operacao == NENHUMA_OPERACAO && resposta.pc == pc;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: Application ID FD_CONTROLE FD_RESPOSTA\n");
        return 1;
    }
    int id = atoi(argv[1]);
    int fd_controle = atoi(argv[2]);
    int fd_resposta = atoi(argv[3]);
    if (id < 1 || id > QUANTIDADE_APLICACOES ||
        fd_controle < 0 || fd_resposta < 0) return 1;
    int pc = 0, n = 0;
    int limite = MAX_ITERACOES;
    /* Reducao EXCLUSIVA para teste: sem a variavel sao 5000 iteracoes. */
    const char *max_teste = getenv("TESTE_MAX_ITERACOES");
    if (max_teste != NULL) {
        int valor = atoi(max_teste);
        if (valor >= 1 && valor <= MAX_ITERACOES) limite = valor;
    }
    int teste = getenv("TESTE_SYSCALL") != NULL;
    int sem_syscall = getenv("TESTE_SEM_SYSCALL") != NULL;
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    setbuf(stdout, NULL);

    /* PC vale 0 antes da primeira iteracao e "limite" ao finalizar.
       Assim sao executadas exatamente MAX_ITERACOES iteracoes normalmente. */
    while (pc < limite) {
        pc++;
        if (!informar_contexto(fd_controle, id, pc, n)) {
            close(fd_controle);
            close(fd_resposta);
            return 1;
        }
        sleep(1);
        Operacao operacao = NENHUMA_OPERACAO;
        if (!sem_syscall && teste && pc == 2 && id == 1) operacao = ENVIAR;
        else if (!sem_syscall && teste && pc == 2 && id == 2) operacao = RECEBER;
        else if (!sem_syscall && !teste && rand() % 100 < 15)
            operacao = rand() % 2 == 0 ? ENVIAR : RECEBER;

        if (operacao != NENHUMA_OPERACAO &&
            !solicitar_syscall(fd_controle, fd_resposta, id,
                              operacao, &pc, &n)) {
            close(fd_controle);
            close(fd_resposta);
            return 1;
        }
        /* RECV pode ter alterado N; atualiza o contexto antes da proxima
         * iteracao. O contexto de SEND tambem fica registrado aqui. */
        if (operacao != NENHUMA_OPERACAO &&
            !informar_contexto(fd_controle, id, pc, n)) {
            close(fd_controle);
            close(fd_resposta);
            return 1;
        }
        if (pc % 100 == 0) printf("[A%d] PC=%d N=%d\n", id, pc, n);
    }
    printf("[A%d] Finalizada (PC=%d, N=%d).\n", id, pc, n);
    int confirmado = avisar_termino(fd_controle, fd_resposta, id, pc, n);
    if (confirmado)
        printf("[A%d] Termino confirmado pelo KernelSim.\n", id);
    close(fd_controle);
    close(fd_resposta);
    return confirmado ? 0 : 1;
}
