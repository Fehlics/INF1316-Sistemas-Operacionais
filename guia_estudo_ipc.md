# Guia de estudo: processos e IPC em Unix/C

Cobre os cinco PDFs que você enviou, na ordem: **Lab 1** (fork/exec), **Lab 2** (memória compartilhada), **Lab 3** (sinais), **Lab 4** (pipes e dup) e **Lab 6** (FIFO). O Lab 5 não veio, então não está aqui. O slide 2 do Lab 2 lista quatro métodos de IPC (memória compartilhada, sinais, pipes e troca de mensagens), então o Lab 5 provavelmente é troca de mensagens.

Cada seção segue a ordem dos slides. Onde um slide tem um erro ou uma imprecisão, marquei com **⚠**. Os códigos dos exercícios com `TODO` estão no `.zip` (pasta `exercicios/`, com o gabarito testado em `gabarito/`). Ao lado de cada tópico indico qual `exNN` treina aquilo.

Convenção: `pai` e `filho` são processos; "fd" é descritor de arquivo.

---

# LAB 1: Criação de processos (`fork()` e `exec()`)

## Slide 1: Título "Criação de Processos"

Um **processo** é um programa em execução. Ele tem:

- um **PID**;
- um espaço de endereçamento privado (text, data, heap, stack);
- uma tabela de descritores de arquivo;
- tratadores de sinais, variáveis de ambiente e outros atributos.

O Unix cria processos em **dois passos**:

1. `fork()` **clona** o processo atual.
2. `exec()` **troca o programa** que o clone está rodando.

Separar os passos é o que torna a shell tão simples. Entre o `fork` e o `exec`, o filho pode reorganizar seus descritores (redirecionamento, pipes) antes de virar outro programa. Você vai usar isso no Lab 4.

## Slide 2: Chamadas de sistema (Process Management)

A tabela do slide (é a do livro do Tanenbaum) tem quatro chamadas.

**`pid = fork()`** cria um filho idêntico ao pai. É chamada **uma vez** e retorna **duas vezes**, uma em cada processo:

| Onde você está | Valor retornado |
|---|---|
| No **pai** | o PID do filho (> 0) |
| No **filho** | `0` |
| Erro (sem recursos) | `-1`, e nenhum filho é criado |

Depois do `fork` os dois seguem executando **a partir da mesma linha**. O filho recebe uma cópia do espaço de endereçamento (na prática, *copy-on-write*: as páginas só são copiadas quando alguém escreve nelas). Por isso variável alterada no filho **não muda no pai**. O filho também herda os descritores abertos, os tratadores de sinal e o ambiente.

```c
pid_t pid = fork();
if (pid < 0)       { perror("fork"); exit(1); }
else if (pid == 0) { /* só o FILHO executa aqui */ }
else               { /* só o PAI executa aqui; pid = PID do filho */ }
```

**`pid = waitpid(pid, &status, options)`** faz o pai esperar um filho terminar.

- `pid = -1` significa "qualquer filho". `pid > 0` significa "esse filho".
- `options = 0` bloqueia. `WNOHANG` não bloqueia e retorna 0 se ninguém terminou.
- `status` é um inteiro **codificado**. Use macros para decodificar:
  - `WIFEXITED(status)` diz se saiu normalmente, e `WEXITSTATUS(status)` dá o valor passado a `exit()` (0 a 255).
  - `WIFSIGNALED(status)` diz se morreu por sinal, e `WTERMSIG(status)` dá qual sinal.
- Se o pai **nunca** chamar `wait`, o filho terminado vira **zumbi** (some do processamento mas continua na tabela de processos). Se o pai morrer antes, o filho é **órfão** e adotado pelo `init` (PID 1).

**`s = execve(name, argv, environp)`** substitui o programa do processo (próximos slides).

**`exit(status)`** termina o processo e devolve o estado ao pai. `exit` descarrega os buffers do stdio; `_exit` não. Use `_exit` no filho quando um `exec` falha, para não descarregar buffers herdados do pai.

## Slide 3: Diagrama `fork()/exec()`

O diagrama mostra o que a shell faz quando você digita um comando:

```
Login Shell --fork()--> Subshell
   |                       |
 wait()  <--- sinal ---  exec(....) --> exit()
```

1. A shell faz `fork()` e cria um subshell (clone dela).
2. O **pai** fica em `wait()`.
3. O **filho** faz `exec(...)` e vira o programa pedido.
4. Quando o programa faz `exit()`, o kernel manda `SIGCHLD` ao pai e o `wait()` retorna.

Note que o sinal do diagrama é o `SIGCHLD`, que você vai ver no Lab 3.

## Slide 4: Esboço de uma criação de processo

```c
pid = fork();
if (pid != 0) {  // Pai
    waitpid(-1, &status, 0);
} else {         // Filho
    exit(3);
}
```

**⚠** O `pid != 0` mistura pai e erro (`-1` cai no ramo do pai). Em código real teste `pid < 0` primeiro. Aqui o pai recebe `status` com o valor 3 codificado. Para ler o 3, use `WEXITSTATUS(status)`.

**Pegadinha clássica:** se o ramo do filho **não** terminar com `exit`, o filho continua executando o código que vem depois do `if/else` e você fica com dois processos rodando o resto do programa. Isso também explica os "dois printfs" que às vezes aparecem em resultados de exercício.

**Pegadinha do stdio:** se você imprime **antes** do `fork` e a saída **não** é um terminal (redirecionou para arquivo, `| tee`, ou está montando o relatório), o `printf` ainda está no buffer e o filho herda uma cópia dele. O texto sai duas vezes. Em terminal a saída é *line-buffered* e o problema não aparece. Solução: `fflush(stdout)` antes do `fork`. (No `ex01` já deixei o `fflush`.)

> Treina: **ex01**.

## Slide 5: Esboço de uma shell

```c
while (TRUE) {
    type_prompt();
    read_command(command, parameters);
    if (fork() != 0) {
        waitpid(-1, &status, 0);      /* pai (shell) espera */
    } else {
        execve(command, parameters, 0);   /* filho vira o comando */
    }
}
```

Pense em `ls -l`: a shell lê `ls -l`, faz `fork`, o filho vira `ls` com `argv = {"ls","-l",NULL}` e a shell espera. Se você digitar `ls -l &`, a única diferença é que a shell **não** chama `waitpid` naquele momento.

**⚠** O `0` no último argumento do `execve` é `envp = NULL`, que funciona no Linux mas não é a forma portável. O normal é passar `environ` (slide 8).

## Slide 6: As variantes de `exec()`

São seis funções. Só a `execve` é uma chamada de sistema; as outras são funções da libc que a chamam. Os sufixos dizem como você passa as coisas:

| Sufixo | Significado | Exemplo |
|---|---|---|
| `l` | argumentos em **lista**, terminada em `NULL` | `execl("/bin/ls","ls","-l",NULL)` |
| `v` | argumentos em **vetor** `char *argv[]` | `execv("/bin/ls", argv)` |
| `p` | procura o executável no **PATH** | `execlp("ls","ls","-l",NULL)` |
| `e` | você passa o **ambiente** (`envp`) explicitamente | `execle(path,"prog",NULL,envp)` |

Regras que dão erro toda vez:

- **`argv[0]` conta.** Convenção: é o nome do programa. `execl("/bin/ls", "ls", "-l", NULL)`: aqui `"ls"` é o `argv[0]`.
- A lista/vetor **termina em `NULL`** (com cast `(char *)NULL` nas versões `l`).
- Em **sucesso o exec nunca retorna**, pois o código antigo deixou de existir. Qualquer linha depois do `exec` só executa se ele **falhou**. Por isso é comum `perror("exec"); _exit(127);` logo depois.
- O **PID não muda**, e os descritores abertos continuam abertos. É isso que faz o redirecionamento da shell funcionar.

**⚠** O exemplo do slide `execle("/usr/bin/monitor","monitor",NULL,"HOME=myhome",NULL)` está errado. O último argumento de `execle` é **um vetor** de strings, não strings soltas. O correto:

```c
char *envp[] = {"HOME=myhome", NULL};
execle("/usr/bin/monitor", "monitor", (char *)NULL, envp);
```

> Treina: **ex02**.

## Slide 7: Argumentos de linha de comando

Quando a shell (ou seu programa, via `exec`) executa `show-arguments 3 5`, o programa recebe:

```
argc = 3
argv[0] = "show-arguments"   argv[1] = "3"   argv[2] = "5"   argv[3] = NULL
```

Tudo chega como **string**. Se precisa do número, use `atoi(argv[1])` ou `strtol`. O código do slide só percorre `argv` imprimindo, e usa `apue.h`, um header do livro Stevens, que você não precisa ter: troque por `<stdio.h>` e `<stdlib.h>`.

Isto é o que o pai controla ao chamar `exec`: se ele faz `execl("./prog","prog","3","5",NULL)`, o filho enxerga `argc = 3`. Você vai usar isso em ex04c (o pai passa o `shmid` ao filho como texto).

## Slide 8: Environment list

Todo processo tem uma **lista de variáveis de ambiente**: um vetor de ponteiros para strings `NOME=valor`, terminado em `NULL`, acessível pela variável global `environ`:

```c
extern char **environ;
for (char **e = environ; *e; e++) puts(*e);   // HOME=/home/davi, PATH=..., etc.
```

Funções mais práticas: `getenv("HOME")`, `setenv("X","1",1)`, `putenv`.

- Com `fork`, o filho **herda** o ambiente do pai.
- `execl/execv/execlp/execvp` (sem `e`) passam o `environ` atual adiante.
- `execle/execve` **substituem** o ambiente pelo vetor que você passar. Por isso o exemplo de `envp` do slide 6 só terá `HOME`.

## Slide 9: Relatório entregável

O relatório é **um único `.txt`**, em ASCII. Para cada exercício, coloque:

1. o enunciado;
2. o código fonte;
3. a linha de compilação e execução;
4. a saída gerada;
5. uma reflexão sobre **por que** a saída foi aquela.

Assunto do e-mail: `[INF1316] Lab # - Nome1, Nome2`, enviado ao monitor e ao professor até a meia-noite do dia do lab + 1.

Dicas práticas: `./prog | tee saida.txt` (com o `fflush` do slide 4!) e `script` ajudam a capturar a saída. Compilar com `gcc -Wall -g -o prog prog.c` e registrar isso.

## Slide 10-11: Perguntas / URL dos Labs

Só o link das aulas práticas do professor, sem matéria nova.

## Slide 12: Exercícios (o que se espera de você)

| # | O que fazer | O que a reflexão precisa dizer |
|---|---|---|
| 1 | pai imprime seu PID e espera; filho imprime o seu e termina | os PIDs são diferentes; o `PPID` do filho é o PID do pai |
| 2 | variável iniciada com 1; filho muda para 5; pai imprime depois do `waitpid` | continua **1** no pai: cada processo tem seu **espaço de endereçamento** (cópia) |
| 3 | filho ordena um vetor de 10, pai imprime antes e depois | o pai **não** vê a ordenação; mesma razão do 2 (só que agora com um vetor inteiro copiado) |
| 4 | filho executa seu `alo` e depois o `echo` | depois do exec o código do filho é **substituído**; o que vem depois do `exec` não roda |

> Treinam: **ex01** (itens 1 a 3) e **ex02** (item 4).

---

# LAB 2: Memória compartilhada

## Slide 1-2: Título e "Métodos de comunicação entre processos"

Processos têm espaços de endereçamento **isolados**. Para cooperar, precisam de um mecanismo de IPC (*inter-process communication*). O curso vai passar por quatro:

1. memória compartilhada (este lab);
2. sinais (Lab 3);
3. pipes (Lab 4) e FIFOs (Lab 6);
4. troca de mensagens (provavelmente o Lab 5).

## Slide 3: Diagrama de memória compartilhada

O desenho mostra dois processos, P1 e P2, cada um com `text / data / heap / stack`. Uma região **shared memory (mapped)** de cada um aponta para o **mesmo bloco físico** no meio. A ideia:

- os endereços **virtuais** podem ser diferentes em P1 e P2;
- as páginas físicas são as mesmas;
- o que um escreve, o outro vê.

## Slide 4: Memória compartilhada (conceito)

- **Mais rápida** forma de IPC: depois de mapeada, ler/escrever é só acessar memória, **sem system call** por acesso.
- **O kernel não sincroniza nada.** Esse é o preço. Se dois processos fazem `*p += 1` ao mesmo tempo, a operação (carrega, soma, guarda) pode se intercalar e você **perde atualizações** (*race condition*). No exemplo do slide 12 a única "sincronização" é o `wait()`. Nos exercícios você vai usar `seq` e espera ativa (ex04c); mais tarde entram semáforos.
- Resolve o problema de "pai e filho quererem acessar a mesma posição": sem isso, o `fork` só dá **cópias**.

## Slide 5: Modelo (ciclo de vida)

1. Um processo **aloca** o segmento (`shmget`). A alocação é em múltiplos do tamanho de página (tipicamente 4 kB).
2. Os outros processos **anexam** (`shmat`), o segmento vira parte do seu espaço de endereçamento.
3. Todos leem/escrevem.
4. No fim, todos **desanexam** (`shmdt`) e **um** processo **remove** o segmento (`shmctl(IPC_RMID)`).

O segmento **não some sozinho** quando o programa termina, pertence ao kernel. Se você esquecer, ele fica vazando: veja com `ipcs -m` e limpe com `ipcrm -m <id>`.

## Slide 6: Alocação com `shmget()`

```c
int shmget(key_t key, size_t size, int shmflg);
```

- **`key`**: identifica o segmento no sistema.
  - `IPC_PRIVATE` sempre cria um segmento **novo** e anônimo. Só serve para processos **parentes** (o filho herda o id pelo `fork`).
  - Uma chave **fixa** (como `8752`, do exercício "Mensagem do Dia") ou gerada por `ftok(caminho, id)` permite que processos **sem parentesco** se encontrem.
- **`size`**: tamanho **mínimo**. O kernel arredonda para páginas.
- **`shmflg`**: permissões (como as de arquivo, ex.: `0600` ou `S_IRUSR|S_IWUSR`) combinadas com flags de criação.
- **Retorno**: um **id** (não é o endereço!) ou `-1` em erro.

## Slide 7: Flags de criação

- `IPC_CREAT`: cria se a chave não existir.
- `IPC_EXCL`: junto com `IPC_CREAT`, **falha** se a chave já existir (garante que você é o dono novo).
- `IPC_NOWAIT`: existe em `<sys/ipc.h>` mas não tem efeito útil no `shmget` (é para filas de mensagens/semáforos).
- As `S_I...` de `<sys/stat.h>` são os bits de permissão de dono/grupo/outros. Exemplo: `S_IRUSR | S_IWUSR` equivale a `0600`.

Padrão típico:

```c
int id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
```

## Slide 8: Anexar com `shmat()`

```c
void *shmat(int shmid, const void *shmaddr, int shmflg);
```

- `shmaddr = NULL` (ou 0): o kernel escolhe o endereço. É o normal.
- `shmflg = 0` (leitura e escrita) ou `SHM_RDONLY`.
- Retorna o **endereço** onde o segmento apareceu.

**⚠** O slide diz "-1 em caso de falha", mas o retorno é um ponteiro: a falha é `(void *)-1`, e é assim que se compara:

```c
int *p = shmat(id, NULL, 0);
if (p == (void *)-1) { perror("shmat"); exit(1); }
```

Depois do `shmat`, `p` é um ponteiro comum: `*p = 8752;`, `p[3]`, ou um cast para `struct`.

Herança: os segmentos anexados **antes do `fork`** ficam anexados no filho (por isso o exemplo funciona). Depois de um **`exec`**, todos os segmentos são **desanexados**. Um filho executado com `exec` precisa fazer seu próprio `shmat`, o que exige que ele saiba o `shmid` (via `argv`) ou a chave (ex04c).

## Slide 9: Desanexar com `shmdt()`

`int shmdt(const void *shmaddr);` recebe o **endereço** retornado pelo `shmat` (não o id). Só desconecta o processo; **não** destrói o segmento.

## Slide 10: Controle com `shmctl()`

`int shmctl(int shmid, int cmd, struct shmid_ds *buf);`

| `cmd` | Faz |
|---|---|
| `IPC_STAT` | preenche `buf` com os atributos do segmento (tamanho, nº de anexos, criador etc.) |
| `IPC_SET` | altera dono, grupo e permissões (`shm_perm`) conforme `buf` |
| `SHM_LOCK`/`SHM_UNLOCK` | trava/destrava o segmento na RAM (sem swap) |
| `IPC_RMID` | **marca para remoção**; o segmento é de fato destruído quando o último processo se desanexar |

Exemplo de `IPC_STAT`:

```c
struct shmid_ds ds;
shmctl(id, IPC_STAT, &ds);
printf("tamanho=%zu anexos=%lu criador=%d\n", ds.shm_segsz,
       (unsigned long)ds.shm_nattch, (int)ds.shm_cpid);
```

## Slide 11: `shmid_ds` e `ipc_perm`

Só a lista de campos.

- `struct shmid_ds`: `shm_perm` (permissões), `shm_segsz` (tamanho em bytes), `shm_lpid` (PID da última operação), `shm_cpid` (PID do criador), `shm_nattch` (**quantos anexos agora**), `shm_atime`/`shm_dtime`/`shm_ctime` (horários do último attach, detach e mudança).
- `struct ipc_perm`: `uid`/`gid` (dono), `cuid`/`cgid` (criador), `mode` (permissão).

`shm_nattch` é o campo mais útil para depurar: se ainda estiver > 0 depois que você esperava que todos tivessem saído, alguém esqueceu o `shmdt`.

## Slide 12-13: Exemplo, pai e filho incrementando o mesmo inteiro

O programa:

1. aloca um `int` compartilhado (`IPC_PRIVATE`) e faz `*p = 8752`;
2. faz `fork`;
3. o **filho** faz `*p += 5` e imprime (**8757**);
4. o **pai** espera com `wait` (é isso que garante a ordem), soma 10 e imprime (**8767**).

A saída do slide (8757 e 8767) confirma que o filho e o pai mexem no **mesmo** inteiro. Compare com o Lab 1: lá o filho mudava `x` e o pai **não** via.

**⚠ Bugs no exemplo:**

- Falta `#include <stdlib.h>` (usa `exit`).
- Depois do `if/else`, **tanto o filho quanto o pai** executam `shmdt` e `shmctl(IPC_RMID)`. O filho já marca o segmento para remoção, então o `shmctl` do pai (no fim) falha silenciosamente com `EINVAL`. O certo é o filho só fazer `shmdt` e **apenas o pai** fazer o `IPC_RMID`.
- Não checa o retorno de `shmget` (o slide só comenta "comparar o retorno com -1" no `shmat`).

> Treina: **ex03** (versão corrigida).

## Slide 14: Alternativa POSIX: `shm_open` / `mmap`

O `shmget/shmat` acima é a API **System V**. A alternativa **POSIX** trata a memória compartilhada como um **arquivo**:

```c
int fd = shm_open("/meu_seg", O_CREAT | O_RDWR, 0600);   // nome começa com '/'
ftruncate(fd, sizeof(int));                                // define o tamanho (obrigatório!)
int *p = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (p == MAP_FAILED) { perror("mmap"); }
*p = 42;
munmap(p, sizeof(int));
close(fd);
shm_unlink("/meu_seg");        // remove (como o IPC_RMID)
```

Vantagem: como é um fd, dá para usar `fstat`, `ftruncate` etc. Desvantagem: precisa de um nome. Em glibc antigas, compile com `-lrt`. **⚠** O slide escreve "unmmap"; a função é `munmap`.

## Slide 15: Perguntas

Sem matéria.

## Slide 16-19: Exercícios

**1) Soma de matrizes.** Três segmentos: A, B (preenchidos) e C (vazia). **Um filho por linha** de C. O slide pede para guardar as matrizes como **vetor de tamanho linha × coluna** (o elemento (i, j) fica em `m[i*COL + j]`), porque um segmento é um bloco contíguo, e ponteiros para linhas (`int **`) **não valem entre processos** (o endereço em uma cópia da memória não é o mesmo em outro). Crie todos os filhos antes de esperar por eles, senão executa em série.

> Treina: **ex04**.

**2) Mensagem do dia.** Dois programas **sem parentesco**: por isso `IPC_PRIVATE` não serve, e ambos usam a mesma chave (8752). O escritor usa `IPC_CREAT`; o leitor não usa (se o escritor não rodou, o `shmget` do leitor falha, o que é bom). O leitor pode anexar com `SHM_RDONLY`. O segmento **persiste** entre as execuções dos dois programas (repare em `ipcs -m`), inclusive depois que o escritor termina; é o comportamento esperado da memória compartilhada System V.

> Treina: **ex05** (escritor e leitor).

**3) Busca paralela.** Um vetor grande de inteiros em memória compartilhada, dividido em fatias, um processo por fatia. Cada um procura na sua fatia e informa a posição **global** (não a relativa à fatia). Um segundo segmento (ou um campo por processo) recebe o resultado, porque o `exit(status)` só leva 8 bits.

> Treina: **ex04b**.

**4) Multiplicação multi-processo.** O diagrama do último slide:

```
P1 --(x, seq1)--> m1        m2 <--(y, seq2)-- P2      pai: res = x * y
```

- P1 e P2 são programas **executados por `exec`**, por isso recebem o `shmid` como argumento e fazem `shmat`.
- Cada um dorme um tempo aleatório, escreve o valor **e só depois** incrementa `seq`. O `seq` é um "aviso de valor novo": o pai só imprime o produto quando **os dois** `seq` avançaram.
- O pai fica em **espera ativa** (loop com `usleep`) olhando os `seq`. Use `volatile` para o compilador não guardar o valor em registrador. Em código de produção use semáforos ou *memory barriers*.

> Treina: **ex04c** (pai e filho; compile os dois).

---

# LAB 3: Sinais

## Slide 1-2: Introdução

Um **sinal** é uma notificação **assíncrona** enviada a um processo pelo sistema operacional. "Assíncrona" quer dizer que ele chega **a qualquer momento**, no meio do que o processo estiver fazendo, como uma interrupção de software.

O diagrama do slide mostra o fluxo:

1. o programa segue o fluxo normal;
2. o sinal chega e o fluxo **desvia** para a rotina de tratamento (*handler*);
3. o handler termina e o fluxo **continua de onde parou**.

Usos: diagnóstico (o processo caiu, e o sinal diz por quê) e uma forma **primitiva** de comunicação entre processos ("acorda", "para", "termina"). Sinais carregam só **o número do sinal**, nenhum dado.

## Slide 3: Sinais (definição)

- São interrupções **geradas por software** (o kernel, em nome de um processo ou de um evento).
- Cada sinal tem uma **ação default**: em geral ser descartado (ignorado), ou terminar o processo.

## Slide 4: Macros em `<signal.h>`

Sinais têm um **nome** (`SIGINT`) e um número. Alguns:

| Sinal | Uso |
|---|---|
| `SIGHUP` | terminal fechou |
| `SIGINT` | interrupção do teclado (Ctrl-C) |
| `SIGQUIT` | Ctrl-\ (termina e gera core dump) |
| `SIGILL` | instrução ilegal |
| `SIGABRT` | `abort()` |
| `SIGKILL` | morte garantida |
| `SIGALRM` | alarme de relógio |
| `SIGCONT` | continuar processo parado |
| `SIGCHLD` | filho parou ou terminou |

**⚠ Números de sinais mudam entre sistemas.** A lista do slide segue a numeração **BSD/macOS** (por exemplo `SIGCHLD = 20`, `SIGCONT = 19`). No **Linux**: `SIGUSR1 = 10`, `SIGUSR2 = 12`, `SIGCHLD = 17`, `SIGCONT = 18`, `SIGSTOP = 19`, `SIGTSTP = 20`. Por isso **use sempre os nomes**, nunca números. `kill -l` lista os do seu sistema.

## Slide 5: `SIGABRT` (sinal 6)

Gerado quando o **próprio processo** detecta um erro interno grave (uma restrição violada) e chama `abort()`. Exemplo do slide: o `malloc()` da libc chama `abort()` quando descobre que suas estruturas internas foram corrompidas (típico de *heap overflow*). O que você vê no terminal é `Aborted (core dumped)`.

## Slide 6: Origem de alguns sinais

O diagrama tem a **origem** de cada sinal:

- **Terminal driver**: `SIGINT`, `SIGQUIT` (teclado).
- **Shell**: `SIGHUP`, `SIGKILL`, `SIGTERM` (comando `kill`).
- **Kernel**: `SIGALRM`, `SIGPIPE`.
- **Gerenciador de memória** (a MMU): `SIGSEGV`.
- **Outro processo**: `SIGUSR1`.

O quadrinho no canto mostra a **máscara de sinais**: um sinal **bloqueado** fica pendente e só é entregue quando o bloqueio sai; um **não bloqueado** é entregue ao processo e vai ao handler. (Não confunda **bloquear** com **ignorar**, veja o slide 11.)

## Slide 7: Tabela completa de sinais

Cada sinal tem uma **ação padrão**:

| Ação | Significado |
|---|---|
| `Term` | processo é terminado |
| `Core` | termina e gera arquivo `core` |
| `Ign` | ignorado |
| `Stop` | o processo **para** (dorme, não termina) |
| `Cont` | continua depois de um `Stop` |

Os mais importantes para os exercícios: `SIGUSR1`/`SIGUSR2` (livres para uso do programador), `SIGSTOP`/`SIGCONT` (parar e continuar), `SIGCHLD` (default: ignorado), `SIGALRM`, `SIGSEGV` e `SIGFPE` (erros do programa).

## Slide 8: Enviando sinais: `kill()` e `raise()`

```c
int kill(pid_t pid, int sig);
int raise(int sig);              // = kill(getpid(), sig): manda para si mesmo
```

O parâmetro `pid` muda o destino:

| `pid` | Destino |
|---|---|
| `> 0` | o processo `pid` |
| `= 0` | todos do **grupo** do processo atual |
| `= -1` | todos os processos para os quais você tem permissão (menos o PID 1) |
| `< -1` | todos do grupo `-pid` |

Retorna 0 em sucesso e `-1` em falha. Dois truques úteis: `kill(pid, 0)` **não envia nada**, só testa se o processo existe e você tem permissão. E, apesar do nome, `kill` só mata se o sinal for de terminar.

## Slide 9: O comando `kill`

```
$ kill -s SIGUSR1 985     # ou:  kill -USR1 985
```

Em outro terminal é assim que você "dispara" seus programas de teste (ex10).

## Slide 10: Sinais pelo teclado

| Teclas | Sinal | Efeito |
|---|---|---|
| Ctrl-C | `SIGINT` | termina |
| Ctrl-Z | `SIGTSTP` | suspende (para) |
| Ctrl-\ | `SIGQUIT` | termina com core dump |

**⚠** O slide diz que Ctrl-\ envia `SIGABRT`. O sinal correto é `SIGQUIT`. O próprio programa `ctrl-c.c` do slide 17 instala handler para `SIGQUIT` para tratar o Ctrl-\, o que confirma.

Só afetam processos em **foreground** (o terminal manda o sinal ao grupo de processos em primeiro plano).

## Slide 11: Manipulação de sinais

Ao receber um sinal, o processo pode:

1. deixar acontecer a **ação default** (`SIG_DFL`);
2. **ignorar** (`SIG_IGN`);
3. **capturar** com um handler.

**⚠** O slide diz "bloquear o sinal (SIG_IGN)". Errado: `SIG_IGN` **ignora**. **Bloquear** é outra coisa (`sigprocmask`): o sinal fica pendente e é entregue depois. Além disso, `SIGKILL` e `SIGSTOP` **não podem ser capturados, ignorados nem bloqueados**; é a garantia de que o administrador sempre consegue matar ou parar um processo.

## Slide 12: Como o handler executa

A função tratadora é chamada **implicitamente** quando o sinal chega. Quando ela retorna, o programa volta **exatamente ao ponto onde foi interrompido**. O handler roda **no mesmo contexto de memória** do processo (variáveis globais são visíveis). Consequências importantes:

- Compartilhar dados com o handler exige variáveis `volatile sig_atomic_t`. Sem `volatile`, o compilador pode guardar a variável em registrador e o laço do `main` nunca vê a mudança.
- Dentro de handlers, use apenas funções **async-signal-safe** (`write`, `_exit`, `kill`...). `printf` **não** é seguro (o sinal pode chegar no meio de outro `printf`). Os slides usam `printf` no handler porque em um programa de brinquedo dá certo; nos exercícios eu usei `write` para você ver o jeito certo.
- Handlers **curtos**: só marcar uma flag e deixar o `main` trabalhar (ex10).

## Slide 13: `signal()`

```c
void (*signal(int signum, void (*func)(int)))(int);
```

Essa declaração assusta, mas é só: "recebe um número de sinal e um ponteiro para função `void f(int)`, e devolve o ponteiro anterior". Com um typedef fica legível: `typedef void (*handler_t)(int); handler_t signal(int, handler_t);`.

- `func` pode ser: uma função sua, `SIG_IGN` ou `SIG_DFL`.
- Retorna o **handler anterior** em caso de sucesso, ou **`SIG_ERR`** em erro. O slide diz "-1"; `SIG_ERR` é isso convertido para ponteiro, e é assim que se compara (`if (signal(...) == SIG_ERR)`).

**Nota de portabilidade:** o comportamento exato de `signal()` variou entre versões do Unix (em algumas, o handler voltava a `SIG_DFL` depois de cada sinal). Em código real o recomendado é `sigaction`:

```c
struct sigaction sa = {0};
sa.sa_handler = meu_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;
sigaction(SIGINT, &sa, NULL);
```

No curso, `signal()` basta, e no Linux/glibc ele já tem comportamento "BSD" (o handler continua instalado).

## Slide 14-16: Configurando handlers

Os três modos de `signal(SIGINT, ...)`:

```c
signal(SIGINT, SIG_IGN);        // ignora Ctrl-C
signal(SIGINT, SIG_DFL);        // volta ao padrão (Ctrl-C termina)
signal(SIGINT, trataCtrlC);     // desvia para a sua função
```

E o slide 16 mostra **um handler para vários sinais** (`SIGINT` e `SIGUSR1` no mesmo `funcaoTratadoraX`): o parâmetro `int sinal` do handler diz **qual** sinal chegou, e um `switch` decide.

## Slide 17: `ctrl-c.c`

```c
p = signal(SIGINT, intHandler);      // p = handler anterior
printf("Endereco do manipulador anterior %p\n", p);
p = signal(SIGQUIT, quitHandler);
for (EVER);                           // #define EVER ;;  (laço infinito)
```

- `intHandler` só imprime "Você pressionou Ctrl-C" (o processo **continua**).
- `quitHandler` imprime e faz `exit(0)`.
- O "endereço do manipulador anterior" impresso é `0` (`SIG_DFL`) na primeira vez: mostra que o padrão é o endereço 0 (ou 1 no caso de `SIG_IGN`). Um detalhe que eu vi testando: quando você roda um programa em background (`&`) em uma shell não interativa, ele **já nasce ignorando SIGINT**, e o valor impresso é `0x1`.
- `for(EVER);` gasta 100% de CPU. Nos meus exercícios usei `pause()`.

**Exercício 1**: rode, teste Ctrl-C e Ctrl-\; depois **remova os `signal()`** e repita. Sem handlers: Ctrl-C termina o processo, e Ctrl-\ termina com `Quit (core dumped)`.

> Treina: **ex06** (inclui o exercício 2).

## Slide 18-20: `pause()`

`int pause(void)` põe o processo para dormir **até chegar um sinal** que o termine ou execute um handler.

- **Exemplo 1** (sem handler): imprime "vou parar..." e trava. Ninguém envia sinal, então você só sai com Ctrl-C (`SIGINT` padrão termina o processo, e "Continuei!" **nunca** aparece).
- **Exemplo 2** (com handler de `SIGUSR1`): em outro terminal, `kill -s SIGUSR1 <pid>`. O handler imprime "Sinal 30/10 recebido" (o número depende do sistema, veja o slide 4), o `pause()` retorna e o programa imprime "Continuei!".

Detalhe: quando o handler foi executado, `pause()` **retorna -1** com `errno = EINTR`. (O slide diz que "em caso de sucesso não retorna"; na prática ela só "retorna" quando um handler rodou.)

**Race condition clássica:** `while (!flag) pause();` tem uma janela: se o sinal chegar **entre** o teste da flag e o `pause()`, você dorme para sempre. A solução correta usa `sigprocmask` + `sigsuspend`. Para os exercícios, o padrão dos slides é aceito.

## Slide 21-22: `alarm()`

```c
unsigned alarm(unsigned segundos);
```

Agenda um **`SIGALRM`** para daqui a `n` segundos.

- **⚠** O slide escreve `SIGALARM`; o nome correto é `SIGALRM`.
- `alarm(0)` cancela.
- Só há **um alarme por processo**: uma nova chamada substitui a anterior, e retorna quantos segundos faltavam.
- Sem handler, o padrão do `SIGALRM` é **terminar** (`Alarm clock`).

O exemplo do slide 22 instala `trataAlarme` e chama `alarm(10)`. O handler imprime e **rearma** `alarm(10)`: temporizador periódico. Como o `main` fica no `for(EVER)`, você vê "10 segundos" a cada 10 s.

## Slide 23: `sleep()`

`unsigned sleep(int seg)` dorme pelo tempo dado **ou até chegar um sinal** não ignorado (retorna quantos segundos faltavam). Ela é **interrompida** por qualquer sinal com handler. Não misture `sleep` e `alarm` no mesmo programa, pois em alguns sistemas os dois usam `SIGALRM`.

## Slide 24: Sinais depois de `fork()` e `exec`

| Situação | O que acontece |
|---|---|
| Depois de **`fork`** | o filho **herda** o tratamento dos sinais (handlers inclusive); pode mudar depois |
| Depois de **`exec`**, sinal **ignorado** | continua **ignorado** |
| Depois de **`exec`**, sinal com **handler** | volta ao **padrão** (o código do handler não existe mais) |

Isso explica o `nohup`: ele coloca `SIGHUP` em `SIG_IGN` e faz `exec`; o programa executado continua imune.

## Slide 25-27: `filhocidio.c`

**O que faz:** `filhocidio tempo programa args...` executa o programa como filho e espera `tempo` segundos; se o filho não terminou, o pai **mata** o filho.

**Como o código do slide funciona:**

- Instala um handler para **`SIGCHLD`**.
- O filho faz o `exec`. O pai lê o tempo e faz `sleep(delay)`.
- **Se o filho termina antes:** o kernel manda `SIGCHLD`, que **interrompe o `sleep`** do pai e roda `childhandler`: ele faz `wait`, imprime "terminated within ... com estado ..." e `exit(0)`.
- **Se o filho demora demais:** o `sleep` acaba, o pai imprime "exceeded limit" e faz `kill(pid, SIGKILL)`.

Os dois exemplos do slide 27 (`sleep5`, que acaba a tempo, e `sleep15`, que estoura o limite de 10 s) mostram os dois caminhos.

**⚠ Problemas do código do slide:**

- `execve(argv[2], 0, 0)` passa `argv = NULL`; melhor `execvp(argv[2], &argv[2])`, que também repassa os argumentos do programa.
- `delay` é lido só depois do `fork`, então há uma pequena corrida: se o filho terminar **antes** do `sscanf`, o handler vê `delay = 0`.
- O handler imprime o `status` **cru** (`exit(3)` apareceria como `768`, já que o código sai deslocado 8 bits). O certo é `WEXITSTATUS(status)`.
- Depois do `kill`, o filho também gera `SIGCHLD`, e por isso o `sleep(1)` do final ("necessário para o SIGCHLD chegar"). Pelo código, você esperaria ver uma linha do handler também nesse caso; o print do slide não mostra, então rode e confira.

**Exercício 3 (ex07)** faz o mesmo com outro desenho: o pai usa `alarm(limite)` + handler de `SIGALRM`, e `waitpid`, que é mais direto e sem essas corridas. Compare os dois.

> Treina: **ex07** (com o helper `dorme.c`, que equivale ao `sleep5`/`sleep15`).

## Slide 29-33: Exercícios

| # | Enunciado | Chave da resposta | Arquivo |
|---|---|---|---|
| 1 | executar `ctrl-c.c` e depois remover os `signal()` | com handler: Ctrl-C só imprime; sem handler: termina | ex06 |
| 2 | interceptar `SIGKILL` | **não dá**: `signal(SIGKILL, ...)` retorna `SIG_ERR`, `errno = EINVAL` | ex06 (extra) |
| 3 | explicar `filhocidio.c` | veja acima | ex07 |
| 4 | dois filhos alternando com `SIGSTOP`/`SIGCONT`, 10 trocas, o pai mata os filhos | o pai é o "escalonador" | ex08 |
| 5 | 4 operações; divisão por zero; capturar `SIGFPE` | veja abaixo | ex09 |
| 6 | monitor de chamadas com `SIGUSR1`/`SIGUSR2` | tarifa em duas faixas | ex10 |
| 7 | 3 programas I/O bound + escalonador Round-Robin (1 s, 2 s, 2 s) | o pai gira `SIGCONT`, `sleep(q)`, `SIGSTOP` | ex11 |

**Sobre o exercício 5 (`SIGFPE`)**, dois pontos que costumam surpreender:

- Divisão **inteira** por zero gera `SIGFPE` no x86 (o sinal se chama "floating point", mas é uma exceção aritmética em geral). Divisão de **`float`/`double`** por zero **não** gera sinal: resulta em `inf`. Para ver o sinal, leia inteiros.
- O handler **não pode simplesmente retornar**: quando ele retorna, a CPU **re-executa a instrução que falhou**, que falha de novo, e o handler roda de novo... laço infinito. Encerre (`_exit`) ou use `siglongjmp` para sair.

**Sobre o exercício 6 (tarifa):** até 60 s, 2 centavos/s; a partir do 2º minuto, 1 centavo/s. Ligação de 1 min 30 s = 60×2 + 30×1 = 150 centavos = **R$ 1,50**. Rode em background (`./prog &`) e dispare com `kill -s SIGUSR1 <pid>` e depois `SIGUSR2`.

**Sobre o exercício 7:** é um "escalonador de processos de brinquedo". `SIGSTOP` tira o processo da CPU; `SIGCONT` devolve. O relatório deve dizer o que você observou: as mensagens dos três processos se **revezam** em blocos de 1 s (P1) e 2 s (P2 e P3), e por que a proporção das linhas impressas acompanha a fatia de tempo.

---

# LAB 4: Pipes e redirecionamento de entrada/saída

## Slide 1: Título

Pipe é o mecanismo por trás do `|` da shell (`ps | wc`). Redirecionamento (`>`, `<`) usa as funções `dup/dup2` que estão neste lab. Os dois se apoiam na mesma ideia: **descritores de arquivo são só números que apontam para "algum lugar"**, e você pode trocar para onde apontam.

## Slide 2-3: O pipe, características

- Um **canal** entre processos **parentes** (pai e filhos, ou irmãos), que precisam **herdar** os descritores pelo `fork`. Por isso pipe comum não conecta dois programas sem parentesco (para isso existe o FIFO do Lab 6).
- Política **FIFO**: o primeiro byte que entra é o primeiro que sai.
- É um **stream de bytes**, sem fronteiras de mensagem. Se o escritor faz duas `write` de 9 bytes, o leitor pode receber 18 bytes de uma vez, ou 5 e 13. Escritor e leitor precisam **combinar o formato** (tamanho fixo, separador, ou tamanho no início). Nos meus exercícios uso mensagens de tamanho fixo (ex15) e `strlen+1` com `'\0'` (ex12).
- Pode haver **vários leitores e escritores**, mas não dá para endereçar: o escritor não escolhe quem lê, e o leitor não sabe quem escreveu (a menos que o dado diga).
- **Leitura é destrutiva**: o dado lido some e nenhum outro processo o vê (ex15 mostra isso).
- **Pipe vazio:** `read` **bloqueia** até chegar dado. **Pipe cheio** (64 KB no Linux): `write` bloqueia.
- **Fim de arquivo:** quando **todas** as pontas de escrita são fechadas, o `read` retorna `0` (EOF). Se todas as pontas de **leitura** são fechadas e alguém escreve, o escritor recebe **`SIGPIPE`**.
- Quando todos fecham o pipe ou terminam, o conteúdo **se perde**: não há persistência.
- É **muito eficiente**: não usa o sistema de arquivos, só um buffer no kernel. As implementações variam entre os Unix (vnode/inode ou streams).
- `write` de até `PIPE_BUF` bytes (4096 no Linux) é **atômico**: não se mistura com escrita de outro processo.

## Slide 4-5: Descritores de arquivo

Cada processo tem sua **tabela de descritores**: um vetor de "índice → entrada". Cada entrada aponta para uma **entrada na tabela de arquivos abertos** do sistema (que guarda *file status flags* e o **offset atual**), e essa aponta para o **v-node** (metadados: tamanho do arquivo, etc.).

- Dois processos que **abrem o mesmo arquivo separadamente** têm entradas separadas e, portanto, **offsets independentes** (figura do slide: P1 e P2).
- Depois de um **`fork`**, pai e filho têm tabelas próprias, mas as entradas apontam para a **mesma** entrada de arquivo aberto, então compartilham o offset. Por isso, com `fork`, o filho consegue escrever no mesmo pipe.
- Os três primeiros são convenção: **0 = stdin** (teclado), **1 = stdout** (tela), **2 = stderr** (tela). `printf` escreve no fd 1; `scanf` lê do fd 0. **Se você mudar para onde o fd 1 aponta, o `printf` vai para outro lugar** sem o programa saber. É assim que a shell faz `>`.

## Slide 6-7: `pipe()`

```c
int pipe(int fd[2]);
```

- `fd[0]` = ponta de **leitura**; `fd[1]` = ponta de **escrita**. (Mnemônico: 0 é "entrada", como stdin; 1 é "saída", como stdout.)
- Retorna 0 em sucesso e `-1` em erro.
- Os dados escritos em `fd[1]` saem em `fd[0]`.
- O pipe **é unidirecional**. Para comunicação nos dois sentidos, crie **dois pipes**.
- O padrão: cria o pipe, faz `fork`, e os dois processos "conversam" por `read`/`write`; ao fim, `close`.

## Slide 8-9: Diagrama e esquema de comunicação

```
pipe(fd) → fork() → [pai: fd[0] fd[1]]  [filho: fd[0] fd[1]]   (os dois têm as duas pontas)
Pai → filho:  pai fecha fd[0] e escreve em fd[1];  filho fecha fd[1] e lê de fd[0]
Filho → pai:  o contrário
```

**Por que fechar a ponta que você não usa?** (1) Para o **EOF** funcionar: se o leitor mantém a ponta de escrita aberta, ele mesmo impede o `read` de retornar 0 e trava para sempre. (2) Para o `SIGPIPE`/`EPIPE` funcionar. (3) Para não vazar descritores. **Esquecer um `close` é o erro nº 1 com pipes.**

## Slide 10: Criando um pipe

```c
int fd[2];
if (pipe(fd) < 0) { puts("Erro ao abrir os pipes"); exit(-1); }
```

## Slide 11: `write()`

`ssize_t write(int fildes, const void *buf, size_t nbyte);`

Retorna o número de bytes escritos. Se for **menor** que `nbyte` ou `-1`, houve problema (**⚠** o slide diz só "difere da quantidade enviada"). Funciona igual para arquivos, pipes, FIFOs e sockets: **tudo é descritor**.

## Slide 12: `read()`

`ssize_t read(int fildes, void *buf, size_t nbyte);`

`nbyte` é o **máximo**. Retorna:

- `> 0`: quantos bytes vieram (**pode ser menos** que `nbyte`, então é preciso ler em laço);
- `0`: **EOF** (todas as pontas de escrita fechadas);
- `-1`: erro.

`read` **não coloca `'\0'`** no fim. Se for imprimir com `%s`, termine a string você mesmo (`buf[n] = '\0'`).

## Slide 13: Exemplo de pipe

Um **único processo** escreve `"uma mensagem"` (12 caracteres + `'\0'` = **13 bytes**, por isso "13 dados escritos") em `fd[1]` e lê de `fd[0]`. Mostra que o pipe é só um buffer do kernel. Só funciona sem `fork` porque a mensagem é pequena: se fosse maior que o buffer (64 KB), o `write` bloquearia para sempre, já que ninguém está lendo.

## Slide 14: Pai escreve para o filho

```c
pipe(fd);
if (fork() == 0) { close(fd[1]); read(fd[0], ...); }    // filho lê
else             { close(fd[0]); write(fd[1], ...); }   // pai escreve
```

**⚠** O comentário do ramo do pai ("`fd[1]` desnecessário") deveria dizer `fd[0]`: é o `fd[0]` que o pai fecha. O código está certo.

> Treina: **ex12** (com os papéis invertidos, como pede o exercício 1).

## Slide 15-16: `dup()` e `dup2()`

- **`int dup(int fd)`** cria uma **cópia** do descritor no **menor número livre**. As duas apontam para o mesmo lugar.
- **`int dup2(int fd1, int fd2)`** faz `fd2` apontar para o mesmo lugar que `fd1`; **fecha `fd2` antes**, se estava aberto. Retorna `fd2` em sucesso e `-1` em erro.

Redirecionar o stdout para um arquivo:

```c
int fd = open("saida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);   // agora printf vai para saida.txt
close(fd);                 // o original já não é necessário
```

Prefira `dup2` a "`close(0); dup(fd)`": o `dup2` é **atômico** e você escolhe o número. O `close` + `dup` só funciona porque o `dup` pega o menor livre, e se o `close` falhar ou outra thread abrir um arquivo no meio, quebra.

## Slide 17: Exemplo com `sort`

```c
pipe(fd);
childpid = fork();
if (childpid == 0) {
    close(0);            // fecha stdin
    dup(fd[0]);          // fd 0 agora é a ponta de leitura do pipe
    execlp("sort", "sort", NULL);
}
```

Como o `exec` **preserva os descritores**, o `sort` lê a "entrada padrão" (fd 0), que na verdade é o pipe. Ele nem sabe disso, e por isso funciona para qualquer programa.

**⚠ Falta um passo importante.** O filho ainda tem **`fd[1]`** aberto (herdado), e continua aberto depois do `exec`. Então o `sort` **nunca vê EOF**: o próprio processo dele mantém a ponta de escrita. O filho precisa fazer `close(fd[0]); close(fd[1]);` depois do `dup`/`dup2`, e o pai precisa **fechar `fd[1]` depois de escrever** (é isso que sinaliza "acabou").

## Slide 18: Exemplo `dup`/`dup2`

O programa:

1. abre `esteArquivo` (vira o fd 3, por exemplo);
2. `close(0)` e `dup(fd)` retorna **0**: o stdin agora é o arquivo;
3. `dup2(fd, 1)` retorna **1**: o stdout agora é o arquivo;
4. dois `printf`.

Na tela **não aparece nada**. Os `printf` foram para o arquivo, e o `cat esteArquivo` do slide mostra "valor de retorno de dup(): 0" e "valor de retorno de dup2(): 1". É exatamente isso que a shell faz com `programa > arquivo`.

## Slide 19-21: Exercícios

**1) Pai lê / filho escreve num pipe** e exibe o resultado. → **ex12**

**2) Ler de um arquivo e gravar em outro por redirecionamento.** O programa só usa `getchar/putchar`; o redirecionamento é feito com `dup2` (stdin ← arquivo de entrada; stdout → arquivo de saída). → **ex13**

**3) `ps | wc`.** O que a shell faz:

1. `pipe(fd)`;
2. filho 1: `dup2(fd[1], 1)`, fecha `fd[0]` e `fd[1]`, `exec ps`;
3. filho 2: `dup2(fd[0], 0)`, fecha `fd[0]` e `fd[1]`, `exec wc`;
4. **pai fecha `fd[0]` e `fd[1]`** e espera os dois.

Se o pai esquecer de fechar `fd[1]`, o `wc` fica esperando o EOF que nunca chega. A saída `28 28 310` são linhas, palavras e bytes. → **ex14** (usei `ls -l | wc -l`; troque para `ps`/`wc` para igualar ao slide).

**4) Dois leitores e um escritor no mesmo pipe.** O escritor dorme metade do tempo dos leitores, então produz o dobro do que eles consomem, e o pipe vai acumulando. Você deve observar que **cada mensagem é consumida por um só leitor**, e que os dois leitores se **alternam** na prática (dependem do escalonador). Sem controle por `SIGSTOP/SIGCONT`, como pede o enunciado. → **ex15**

---

# LAB 6: Named pipes (FIFO)

## Slide 1: Título

O pipe comum exige parentesco. O **FIFO** (*named pipe*) resolve isso dando ao pipe um **nome no sistema de arquivos**: qualquer processo que conheça o nome (e tenha permissão) pode abrir.

## Slide 2: Named pipe (FIFO)

- Um FIFO permite comunicação entre **quaisquer dois processos**.
- É um arquivo **especial** visível no sistema de arquivos (`ls -l` mostra `p` no início: `prw-------`).
- Os dados **não são gravados em disco**: passam por um buffer no kernel. O nome serve só de **ponto de encontro**.
- O controle de acesso é o de arquivos (bits `rwx`, filtrados pelo `umask`).
- Para remover: `rm nome` (ou `unlink()` no programa).

**⚠** O slide diz que "o FIFO persiste dados além do processo que o criou". Na verdade só o **nome** (o arquivo especial) persiste. Os **dados** ficam no buffer do kernel e se perdem quando ninguém tem o FIFO aberto.

## Slide 3: Diagrama

P1 faz `write()` para um **buffer no espaço do kernel** e P2 faz `read()` desse buffer. Os dados são **não estruturados**, exatamente como no pipe comum (stream de bytes).

## Slide 4: Abrindo as duas pontas

O FIFO precisa estar aberto **nas duas pontas** (leitura e escrita) antes de qualquer transmissão.

- **Modo normal:** `open()` **bloqueia** até que a outra ponta também abra.
- **Modo não bloqueante** (`O_NONBLOCK`): `open` para **leitura** retorna já, mesmo sem escritor.

É a fonte de quase todos os "meu programa travou" com FIFO: **o `open` está esperando o par**.

## Slide 5: Criando FIFOs na shell

```
$ mkfifo fpipe
$ grep "\.c" < fpipe &       # leitor em background: bloqueia no open, esperando um escritor
$ ls -ls ../IPC/reserva > fpipe      # escritor: a saída do ls vai para o FIFO
```

Quando o `ls` abre o FIFO para escrita, o `grep` (que estava bloqueado) destrava, recebe a lista e imprime só as linhas com `.c`. Quando o `ls` termina (fecha a ponta), o `grep` vê EOF e termina (`[1]+ Done`). O `ls -ls` mostra `prw-r--r--`: o `p` indica um FIFO, e o tamanho é sempre **0** (os dados não ficam no arquivo).

## Slide 6-7: `mkfifo()` e o programa que cria

```c
int mkfifo(const char *filename, mode_t mode);   // <sys/stat.h>
```

`mode`: permissões (`S_IRUSR | S_IWUSR` = `0600`), reduzidas pelo `umask`. Retorna 0 ou `-1`. O programa do slide cria `minhaFifo`, e a **segunda execução falha**, porque o arquivo já existe (`errno = EEXIST`). Por isso, em programas reais, aceite `EEXIST` como "tudo bem":

```c
if (mkfifo(FIFO, 0600) < 0 && errno != EEXIST) { perror("mkfifo"); exit(1); }
```

**⚠** Os programas dos slides 11 e 12 usam `access(FIFO, F_OK)` para checar antes de criar. Funciona, mas tem uma janela de corrida entre a checagem e a criação; o `errno != EEXIST` acima é mais robusto.

## Slide 8: Usando o FIFO

Escritor: `open`, `write`, `close`. Leitor: `open`, `read` em laço até `0`, `close`.

**⚠ Erros de digitação no slide** (não copie): `open("minhaFifo", "w")` está errado. `open` recebe **flags inteiras** (`O_WRONLY`, `O_RDONLY`), e `"w"/"r"` são do `fopen`. O `write` está sem `)` e usa `c` sem declarar. A mensagem de erro do leitor diz "para escrita" (copiada do escritor).

O `read` retorna `0` quando **todos os escritores fecharam**: é o EOF, e o laço `while (read(...) > 0)` termina.

> Treina: **ex16**.

## Slide 9: Problemas com FIFO

- **Não abra o mesmo FIFO para leitura e escrita no mesmo processo** (`O_RDWR`). O comportamento não é definido pelo POSIX, e no Linux o processo se torna seu próprio escritor, então **nunca vê EOF**.
- Para comunicação **bidirecional**, use **dois FIFOs** (um por sentido). Isso é o exercício 3.
- A alternativa "não recomendada" (abrir num sentido, fechar e reabrir no outro) é frágil.

## Slide 10: FIFO não bloqueante

| Chamada | Comportamento |
|---|---|
| `open(p, O_RDONLY)` | **bloqueia** até algum processo abrir para escrita |
| `open(p, O_RDONLY \| O_NONBLOCK)` | **retorna imediatamente**, mesmo sem escritor |
| `open(p, O_WRONLY)` | **bloqueia** até algum processo abrir para leitura |
| `open(p, O_WRONLY \| O_NONBLOCK)` | retorna na hora, mas **falha (`-1`, `errno = ENXIO`)** se ninguém abriu para leitura |

## Slide 11: Leitura sem bloqueio

O programa: cria o FIFO se não existir, abre com `O_RDONLY | O_NONBLOCK`, e lê em laço.

Saída do slide: "Abrindo FIFO / Começando a ler... / Fim da leitura" **na hora**. Como não há nenhum escritor, o `read` retorna `0` (EOF) imediatamente, e o laço termina. (Se houvesse escritor conectado mas sem dados no momento, o `read` retornaria `-1` com `EAGAIN`, e o laço `> 0` também terminaria, então **leitura não bloqueante exige tratar `EAGAIN`** ou usar `poll`/`select`.)

## Slide 12: Escrita sem bloqueio

Mesma estrutura, com `O_WRONLY | O_NONBLOCK`. Como não há leitor, o `open` **falha** e o programa imprime "Erro ao abrir a FIFO minhaFifo" (é o print do slide).

## Slide 13: "O mesmo programa, com bloqueio"

Sem `O_NONBLOCK`, os dois lados esperam um ao outro. Os dois experimentos do slide:

1. **Leitor primeiro** (em background): bloqueia no `open`. Depois o escritor abre, e os dois destravam: o escritor escreve "Melancia sem caroço" (sem `\n`, por isso "Fim da leitura" aparece colado na mensagem) e o leitor imprime.
2. **Escritor primeiro** (em background): bloqueia no `open` esperando leitor. O leitor abre e o resultado é o mesmo.

A **ordem das linhas** na tela varia entre execuções, porque depende do escalonamento e do buffer do stdout.

## Slide 14-17: Exercícios

**1) Dois terminais.** Terminal 1: programa em laço lendo do FIFO e escrevendo na tela. Terminal 2: programa lendo do teclado e escrevendo no FIFO. Experimente iniciar só um dos lados e veja o `open` bloqueando. → **ex16**

**2) Pai cria FIFO + dois filhos escritores; pai faz `waitpid` e só então lê.** ⚠ **Armadilha:** os filhos fazem `open(O_WRONLY)`, que bloqueia até haver um **leitor**. Se o pai fizer `waitpid` **antes** de abrir o FIFO para leitura, os filhos travam no `open`, o pai trava no `waitpid`, e você tem **deadlock**. A saída: o pai abre o FIFO para leitura com `O_RDONLY | O_NONBLOCK` **antes** de criar os filhos. Aí os `open` dos filhos passam, eles escrevem e terminam, e o pai lê tudo depois (o buffer do FIFO aguenta). → **ex17**

**3) Servidor e cliente com duas FIFOs** (uma para requisições, uma para respostas). O servidor roda em background e devolve cada palavra em maiúsculas. Detalhes que importam:

- **Ordem de abertura idêntica nos dois lados** (REQ primeiro, RESP depois), senão deadlock: cada um espera o outro abrir a FIFO que ele mesmo ainda não abriu.
- Quando o **último cliente** fecha a REQ, o `read` do servidor retorna 0. O servidor então **fecha e reabre** as duas FIFOs, para aceitar o próximo cliente.
- `signal(SIGPIPE, SIG_IGN)` no servidor: um cliente que some não deve derrubá-lo.
- **Limitação do desenho:** a FIFO de resposta é **uma só**, então, com vários clientes ao mesmo tempo, a resposta pode ir para o cliente errado. Uma melhoria: o cliente cria um FIFO próprio (por exemplo `resp_<pid>`) e manda o nome junto da requisição. Escritas de até `PIPE_BUF` bytes são atômicas, então as requisições de clientes diferentes não se misturam por dentro.

Execute os clientes em **terminais diferentes**, como diz o slide. → **ex18** (servidor e cliente).

---

# Resumo comparativo

| | Memória compartilhada | Sinais | Pipe | FIFO |
|---|---|---|---|---|
| Parentesco necessário | não (com chave fixa) / sim (`IPC_PRIVATE`) | não | **sim** | não |
| Tem nome | chave (`key_t`) | PID | não | caminho no sistema de arquivos |
| Dados | região de bytes livre | só o nº do sinal | stream de bytes | stream de bytes |
| Sincronização | **nenhuma** (você faz) | assíncrono | leitura bloqueia se vazio | idem + `open` bloqueia |
| Persiste depois que os processos saem | **sim** (`ipcs`, `ipcrm`) | não | não | só o nome, não os dados |
| Velocidade | a maior (sem syscall por acesso) | rápido, mas sem payload | boa | boa |
| Direção | qualquer | um → um (ou grupo) | unidirecional | unidirecional |

# Como usar os exercícios

Abra a pasta `exercicios/` do `.zip`. Cada arquivo tem no topo um comentário com o **objetivo e a saída esperada**, e as partes para você preencher estão marcadas com `TODO` (com dica) e `>>> escreva seu codigo aqui <<<`. Tudo é resolvido no `gabarito/`, e todas as versões resolvidas foram compiladas com `-Wall -Wextra` e executadas.

| Arquivo | Lab | Conceito principal |
|---|---|---|
| `ex01_fork_wait` | 1 | `fork`, `waitpid`, `WEXITSTATUS`, memória separada |
| `ex02_fork_exec` | 1 | `execl`/`execvp`, código que só roda se o `exec` falha |
| `ex03_shm_contador` | 2 | `shmget/shmat/shmdt/shmctl` com `fork` |
| `ex04_shm_matriz` | 2 | vetores compartilhados, um filho por linha |
| `ex04b_shm_busca_paralela` | 2 | divisão de trabalho, posição global |
| `ex04c_shm_produto_pai/filho` | 2 | `exec` + shm por `shmid` no `argv`, `seq` e espera ativa |
| `ex05_shm_msg_escritor/leitor` | 2 | chave fixa entre processos sem parentesco |
| `ex06_sinal_ctrlc` | 3 | `signal`, handler, `sig_atomic_t`, `SIGKILL` não captura |
| `ex07_timeout_filhocidio` | 3 | `alarm` + `SIGALRM` + `kill` no filho |
| `ex08_sigstop_sigcont` | 3 | alternar dois filhos |
| `ex09_sigfpe` | 3 | `SIGFPE` e por que o handler não retorna |
| `ex10_monitor_ligacoes` | 3 | `SIGUSR1/2`, flags, cálculo de tarifa |
| `ex11_escalonador_rr` | 3 | Round-Robin com `SIGSTOP/SIGCONT` |
| `ex12_pipe_pai_filho` | 4 | `pipe`, fechar pontas, EOF |
| `ex13_dup2_redirect` | 4 | `dup2` para stdin/stdout |
| `ex14_pipeline` | 4 | `ls -l \| wc -l` como a shell |
| `ex15_pipe_leitores` | 4 | vários leitores, consumo destrutivo |
| `ex16_fifo_escritor/leitor` | 6 | `mkfifo`, `open` bloqueante |
| `ex17_fifo_pai_filhos` | 6 | deadlock e `O_NONBLOCK` |
| `ex18_fifo_servidor/cliente` | 6 | duas FIFOs, reabertura, `SIGPIPE` |

**Ordem sugerida:** siga a numeração. Para cada exercício: leia o cabeçalho, preencha os TODOs, compile com `gcc -Wall -Wextra -g`, rode, e **antes de olhar o gabarito** responda em voz alta a "pergunta" que aparece nos comentários (ex01: por que `x` continua 1? ex09: por que `_exit`?). É a parte da reflexão que o relatório pede.

**Limpeza:** `ipcs -m` (e `ipcrm -m <id>`) para restos de memória compartilhada; `rm minhaFifo fifo_*` para FIFOs.
