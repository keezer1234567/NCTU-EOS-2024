CC = gcc
CFLAGS = -Wall -g
TARGETS = game guess

all: clean $(TARGETS)

game: game.c
	@$(CC) $(CFLAGS) -o game game.c

guess: guess.c
	@$(CC) $(CFLAGS) -o guess guess.c

clean:
	@rm -f $(TARGETS)
