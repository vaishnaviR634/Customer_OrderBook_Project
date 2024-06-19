CC = gcc
CFLAGS = -g
DEPS = Main.o OrderBook.o

orderbook: $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(DEPS)

Main.o: Main.c OrderBook.h
	$(CC) $(CFLAGS) -c -o $@ $<

OrderBook.o: OrderBook.c OrderBook.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o orderbook
