CC = gcc

CFLAGS = -o
OFLAGS = -Wall -g -c

OFILES = pqs.o
PROJECT = PQS

all: $(OFILES)
		$(CC) $(CFLAGS) $(PROJECT) $(OFILES) -pthread

pqs.o: pqs.c pqs.h
		$(CC) $(OFLAGS) pqs.c

clean:
		rm -f $(OFILES) $(PROJECT)
