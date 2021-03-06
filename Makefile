OBJS = $(obj)/main.o $(obj)/trie.o $(obj)/StringBuffer.o

bin = bin
obj = obj
src = src
EXE = $(bin)/thubi

LIBS =

COMPILE = $(CC) $(CFLAGS) -c $< -o $@

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $+ $(LIBS)

$(obj)/main.o: $(src)/main.c $(src)/trie.h $(src)/StringBuffer.h
	$(COMPILE)
$(obj)/trie.o: $(src)/trie.c $(src)/trie.h
	$(COMPILE)
$(obj)/StringBuffer.o: $(src)/StringBuffer.c $(src)/StringBuffer.h
	$(COMPILE)


clean:
	-rm -f $(EXE) $(obj)/*.o

.PHONY: clean
