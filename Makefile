# Makefile for nloc

CFLAGS:=-O3 -static
nloc: nloc.c
	$(CC) $(CFLAGS) -o nloc nloc.c

clean:
	rm -f *.html *.1 nloc

install: nloc
	cp nloc nloc-test $(HOME)/bin/

dist: nloc-$(VERS).tar.gz
