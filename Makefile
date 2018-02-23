OBJS = $(objdir)/main.o

bindir = bin
objdir = obj
srcdir = src
EXE = $(bindir)/thubi

LIBS =

COMPILE = $(CC) $(CFLAGS) -c $< -o $@

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $+ $(LIBS)

$(objdir)/main.o: $(srcdir)/main.c
	$(COMPILE)

clean:
	-rm -f $(EXE) $(objdir)/*.o

.PHONY: clean
