TARGETS=mysh
# CFLAGS=-std=gnu99
# CC=gcc

.PHONY: cleanall cleanobj

all: $(TARGETS)

Mytar: mysh.o

cleanobj:
	$(RM) -f mysh.o

clean:
	$(RM) -f $(TARGETS)
