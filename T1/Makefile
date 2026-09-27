CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Werror -pedantic

all: Simulador KernelSim InterController Application Testes Validar5000

Simulador: Simulador.c simulador_pausa.c util.c trabalho.h simulador_pausa.h util.h
	$(CC) $(CFLAGS) Simulador.c simulador_pausa.c util.c -o $@

KernelSim: KernelSim.c kernel_filas.c util.c trabalho.h kernel_filas.h util.h
	$(CC) $(CFLAGS) KernelSim.c kernel_filas.c util.c -o $@

InterController: InterController.c util.c trabalho.h util.h
	$(CC) $(CFLAGS) InterController.c util.c -o $@

Application: Application.c util.c trabalho.h util.h
	$(CC) $(CFLAGS) Application.c util.c -o $@

Testes: $(wildcard testes/*.c) util.c util.h testes/testes.h testes/apoio.h trabalho.h
	$(CC) $(CFLAGS) testes/*.c util.c -o $@

Validar5000: validacao/Validar5000.c trabalho.h
	$(CC) $(CFLAGS) validacao/Validar5000.c -o $@

test: all
	./Testes

validar: all
	./Validar5000

clean:
	rm -f Simulador KernelSim InterController Application Testes Validar5000

.PHONY: all test validar clean
