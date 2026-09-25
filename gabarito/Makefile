CC     = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -g -O0
SRCS   = $(wildcard *.c)
BINS   = $(SRCS:.c=)

all: $(BINS)

clean:
	rm -f $(BINS) minhaFifo fifo_filhos fifo_req fifo_resp entrada.txt saida.txt
