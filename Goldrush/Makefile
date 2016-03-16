CC = gcc

CFLAGS = -o
OFLAGS = -Wall -g -c

OFILES = goldrush.o
PROJECT = GR

all: $(OFILES)
        $(CC) $(CFLAGS) $(PROJECT) $(OFILES) -pthread

goldrush.o: goldrush.c goldrush.h
        $(CC) $(OFLAGS) goldrush.c

clean:
        rm -f $(OFILES) $(PROJECT)
