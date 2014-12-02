CC=gcc
CFLAGS=-g -Wall #--std=gnu99 -D__GNU_SOURCE #-Iheaders
LDFLAGS=

SRC=$(wildcard *.c)
OBJ=$(addprefix $(OUTDIR)/, $(SRC:.c=.o))
OUTDIR=bin

PROG=my_shell


all: compile

-include $(OBJ:%.o=%.d)

compile: $(OUTDIR) $(OBJ)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ)

$(OUTDIR):
	mkdir $(OUTDIR)
	
$(OUTDIR)/%.o: %.c
	$(CC) -c -MD $(CFLAGS) $< -o $@
	


clean: 
	-rm -rf $(OUTDIR) 
	
.PHONY: clean all
