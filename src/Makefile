PROG= main

all: build

FLAGS=-I ./include -L /usr/local/lib -lpthread -ltcmalloc_minimal -g -O2
FILES=url_trie.c fields_trie.c thpool.c http_parser.c handler.c awsr.c main.c

build:
	$(CC) $(FILES) $(FLAGS) -o $(PROG)

clean:
	rm $(PROG)
