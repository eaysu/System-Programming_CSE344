CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lm

all: PideShop HungryVeryMuch

PideShop: pideShop.o
	$(CC) $(CFLAGS) -o PideShop pideShop.o $(LDFLAGS)

HungryVeryMuch: hungryVeryMuch.o
	$(CC) $(CFLAGS) -o HungryVeryMuch hungryVeryMuch.o

PideShop.o: pideShop.c
	$(CC) $(CFLAGS) -c pideShop.c

HungryVeryMuch.o: hungryVeryMuch.c
	$(CC) $(CFLAGS) -c hungryVeryMuch.c

clean:
	rm -f *.o *.log PideShop HungryVeryMuch 

.PHONY: all clean
