# T1 — Simulador de escalonamento

Simulador em C com seis aplicações (`A1`–`A6`), um núcleo simulado e um controlador de interrupções. Utiliza somente os mecanismos estudados na disciplina: `fork`, `exec`, `waitpid`, sinais, pipes, vetores circulares e Round Robin. Não utiliza threads, `poll`, `select` ou scripts `.sh`.

## Organização

| Arquivos | Responsabilidade |
|---|---|
| `Simulador.c` | Criação, acompanhamento e encerramento dos processos Unix. |
| `simulador_pausa.c/.h` | Ctrl+Z, fotografia dos PCBs e retomada. |
| `KernelSim.c` | Escalonamento, syscalls, estados e término. |
| `kernel_filas.c/.h` | Filas FIFO e buffers dos três pipes bidirecionais simulados. |
| `Application.c` | Contador `PC`, valor `N`, SEND/RECV e aviso de término. |
| `InterController.c` | IRQ0 periódico e IRQ1/IRQ2 probabilísticos. |
| `util.c/.h` | Envio/leitura de mensagens por pipes e intervalo de testes. |
| `trabalho.h` | Tipos e mensagens compartilhados pelos processos. |
| `testes/` | Oito testes rápidos em C. |
| `validacao/` | Validação independente de 5.000 iterações por aplicação. |

## Compilar e executar

No Linux, dentro de `T1`:

```bash
make                  # compila os quatro programas e os dois executáveis de teste
make test             # oito testes rápidos
make validar          # três cenários de 5.000 iterações por aplicação
./Simulador           # execução normal: Ctrl+Z pausa/retoma, Ctrl+C encerra
make clean            # apaga somente executáveis compilados
```

O `Makefile` apenas reúne comandos `gcc` — nenhum script `.sh` é necessário. Caso `make` não esteja disponível, os comandos equivalentes são:

```bash
gcc -Wall -Wextra Simulador.c simulador_pausa.c util.c -o Simulador
gcc -Wall -Wextra KernelSim.c kernel_filas.c util.c -o KernelSim
gcc -Wall -Wextra InterController.c util.c -o InterController
gcc -Wall -Wextra Application.c util.c -o Application
gcc -Wall -Wextra testes/*.c util.c -o Testes
gcc -Wall -Wextra validacao/Validar5000.c -o Validar5000
```

Os executáveis não fazem parte do código-fonte versionado: **recompile após atualizar o repositório**.

## Funcionamento resumido

O Simulador cria as seis aplicações suspensas, o KernelSim e o InterController. A cada IRQ0, o KernelSim aplica Round Robin usando `SIGSTOP`/`SIGCONT`. SEND e RECV bloqueiam a aplicação e entram em filas FIFO distintas; IRQ2 e IRQ1 atendem, respectivamente, o primeiro pedido pendente de escrita e leitura. Cada par dispõe de dois buffers circulares, um por direção; RECV em buffer vazio devolve `N=0`.

O Linux preserva o contexto real dos processos interrompidos; o KernelSim guarda no PCB o contexto simulado (`PC`, `N` e operação/endereço da syscall). Cada aplicação confirma seu término com o KernelSim, e o Simulador recolhe seus filhos com `waitpid`.

Na pausa com `Ctrl+Z`, o Simulador interrompe o controlador e solicita ao KernelSim uma fotografia dos seis PCBs. Outro `Ctrl+Z` retoma a execução. `Ctrl+C` encerra o conjunto.

## Testes

`make test` executa oito verificações: criação de processos, Round Robin, filas FIFO, respostas de syscalls, término, buffers bidirecionais, pausa/retomada e contexto.

`make validar` executa **três cenários independentes** de 30.000 iterações (6 × 5.000): sem syscall, com SEND/RECV e com pausa/retomada. Esse teste diminui **somente os intervalos de espera** por variáveis de ambiente; a execução normal mantém `sleep(1)` e IRQ0 a cada 500 ms. Os eventos `PC=1` até `PC=5000` são auditados individualmente, além do término e recolhimento dos seis processos. Para executar apenas um cenário, use `./Validar5000 1`, `2` ou `3`.

As restrições, critérios e resultados anteriores estão registrados em `validacao/LEIA-ME-5000.txt` e `validacao/RESULTADOS-LOCAIS.txt`. O relatório final da disciplina deve explicar as decisões e apresentar os resultados obtidos na máquina de entrega.
