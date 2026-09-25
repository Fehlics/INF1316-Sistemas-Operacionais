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

// TODO: Escreva trata_usr1 (flag_inicio = 1) e trata_usr2 (flag_fim = 1).
//       Handlers curtos: so marcam flags; o trabalho "de verdade" e feito no main.
// >>> escreva seu codigo aqui <<<

static long custo_centavos(long seg)
{
    // TODO: Devolva o custo em centavos para 'seg' segundos de ligacao.
    // >>> escreva seu codigo aqui <<<
}

int main(void)
{
    time_t t0 = 0;
    int em_chamada = 0;

    // TODO: Instale os handlers: SIGUSR1 -> trata_usr1, SIGUSR2 -> trata_usr2.
    // >>> escreva seu codigo aqui <<<

    printf("monitor pronto, pid=%d\n", getpid());
    fflush(stdout);

    for (;;) {
        pause();
        if (flag_inicio) {
            flag_inicio = 0;
            // TODO: Guarde o instante inicial em t0 (time(NULL)), marque em_chamada = 1, avise.
            // >>> escreva seu codigo aqui <<<
        }
        if (flag_fim) {
            flag_fim = 0;
            // TODO: Se nao ha chamada em curso, avise e continue. Senao calcule a duracao,
            //       o custo (custo_centavos) e imprima "Chamada de Ns custou R$ X,YY"; em_chamada = 0.
            // >>> escreva seu codigo aqui <<<
            fflush(stdout);
        }
    }
}
