MYLIBDIR = mylib
MYLIB    = $(MYLIBDIR)/mylib.a
OBJS1    = server.o sessionman.o
OBJS2    = client.o session.o
CFLAGS   = -I$(MYLIBDIR)

all: bin bin/s bin/c

bin:
	mkdir $@

bin/s: $(OBJS1)
	$(CC) -o $@ $^ $(MYLIB) -lncurses

bin/c: $(OBJS2)
	$(CC) -o $@ $^ $(MYLIB) -lncurses

server.o: sessionman.h
client.o: session.h

clean:
	$(RM) bin/s bin/c $(OBJS1) $(OBJS2) *~
