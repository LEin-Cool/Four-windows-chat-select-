
SRCS = $(wildcard *.c)
TARGETS = $(SRCS:%.c=%)
all:$(TARGETS)
%:%.c	
	gcc -o $@ $< -lpthread;
.PHONY: rebuild clean
rebuild:clean all 
clean:
	rm -rf $(TARGETS)
