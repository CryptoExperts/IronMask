CC = clang
RM = rm -f
CFLAGS = -Wall -Wextra -O3 -march=native -pthread -mlzcnt -gdwarf-4
LDLIBS = -lm -lgmp

SRC = circuit.c coeffs.c combinations.c constructive.c constructive-mult.c \
	  list_tuples.c main.c parser.c utils.c NI.c SNI.c freeSNI.c IOS.c PINI.c RP.c RPC.c RPE.c \
	  trie.c verification_rules.c failures_from_incompr.c \
	  constructive-mult-compo.c dimensions.c vectors.c hash_tuples.c CNI.c CRP.c CRPC.c
OBJ = $(SRC:.c=.o)

all: ironmask

ironmask: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean mrproper

clean:
	$(RM) *.o

mrproper: clean
	$(RM) -rf ironmask
