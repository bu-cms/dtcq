IDIR=.
CC=g++
CFLAGS=-I$(IDIR) -std=c++11 -I./dspatch/include 

BINDIR=bin
ODIR=obj
SRCDIR=src
INCDIR=interface
LIBS=-lm

# _OBJ = FIFO.o
# OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Set up output directories
$(shell   mkdir -p $(BINDIR))
$(shell   mkdir -p $(ODIR))

$(ODIR)/%.o: $(SRCDIR)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC


.PHONY: clean


demo_fifo: 
	$(CC) src/demo_fifo.cc -o $(BINDIR)/demo_fifo $^ $(CFLAGS) $(LIBS) ./dspatch/build/libDSPatch.so -I./include

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(BINDIR)/*

