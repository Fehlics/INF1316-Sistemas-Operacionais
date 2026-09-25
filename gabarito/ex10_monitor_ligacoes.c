/* ex10_monitor_ligacoes.c  --  Lab 3 (exercicio 6)
 * Rode em background:  ./ex10_monitor_ligacoes &
 *   kill -s SIGUSR1 <pid>   -> inicio da chamada
 *   kill -s SIGUSR2 <pid>   -> fim da chamada (mostra o custo)
 * Tarifa: 2 centavos/s ate 60 s; 1 centavo/s de 60 s em diante.
 * (1m30 = 60*2 + 30*1 = 150 centavos = R$1,50)
 */
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t flag_inicio = 0, flag_fim = 0;

/* [resolvido] Escreva trata_usr1 (flag_inicio = 1) e trata_usr2 (flag_fim = 1). */
static void trata_usr1(int s) { (void)s; flag_inicio = 1; }
static void trata_usr2(int s) { (void)s; flag_fim = 1; }

static long custo_centavos(long seg)
{
    /* [resolvido] Devolva o custo em centavos para 'seg' segundos de ligacao. */
    if (seg <= 60) return 2 * seg;
    return 2 * 60 + (seg - 60) * 1;
}

int main(void)
{
    time_t t0 = 0;
    int em_chamada = 0;

    /* [resolvido] Instale os handlers: SIGUSR1 -> trata_usr1, SIGUSR2 -> trata_usr2. */
    signal(SIGUSR1, trata_usr1);
    signal(SIGUSR2, trata_usr2);

    printf("monitor pronto, pid=%d\n", getpid());
    fflush(stdout);

    for (;;) {
        pause();
        if (flag_inicio) {
            flag_inicio = 0;
            /* [resolvido] Guarde o instante inicial em t0 (time(NULL)), marque em_chamada = 1, avise. */
            t0 = time(NULL);
            em_chamada = 1;
            puts("Chamada iniciada.");
        }
        if (flag_fim) {
            flag_fim = 0;
            /* [resolvido] Se nao ha chamada em curso, avise e continue. Senao calcule a duracao, */
            if (!em_chamada) { puts("Fim sem inicio: ignorado."); continue; }
            long s = (long)(time(NULL) - t0);
            long c = custo_centavos(s);
            printf("Chamada de %lds custou R$ %ld,%02ld\n", s, c / 100, c % 100);
            em_chamada = 0;
            fflush(stdout);
        }
    }
}
