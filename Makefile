IDIR=.
CC=g++
CFLAGS=-I$(IDIR) -std=c++11 -I./dspatch/include

BINDIR=bin
ODIR=obj
SRCDIR=src
INCDIR=interface
LIBS=-lm

_OBJ = FComponent.o  EventBoundaryFinder.o EventBoundaryFinderStreamAligned.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Set up output directories
$(shell   mkdir -p $(BINDIR))
$(shell   mkdir -p $(ODIR))

$(ODIR)/%.o: $(SRCDIR)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC -I./include


.PHONY: clean


demo_evtboundary: $(OBJ)
	$(CC) src/demo_evtboundary.cc -o $(BINDIR)/demo_evtboundary $^ $(CFLAGS) $(LIBS) ./dspatch/build/libDSPatch.so.7 -I./include
demo_fifo:
	$(CC) src/demo_fifo.cc -o $(BINDIR)/demo_fifo $^ $(CFLAGS) $(LIBS) ./dspatch/build/libDSPatch.so.7 -I./include

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(BINDIR)/*

