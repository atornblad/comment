# Makefile
# Author: Anders Tornblad
# Date: 2017-08-30

SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

comment: $(OBJECTS)
	gcc -o comment $(OBJECTS)

comment.o: comment.c $(HEADERS)
	comment comment.c
	gcc -Wall -std=c99 -c comment.c

comment_c.o: comment_c.c comment_c.h
	comment comment_c.c comment_c.h
	gcc -Wall -std=c99 -c comment_c.c

comment_sh.o: comment_sh.c comment_sh.h
	comment comment_sh.c comment_sh.h
	gcc -Wall -std=c99 -c comment_sh.c

comment_tex.o: comment_tex.c comment_tex.h
	comment comment_tex.c comment_tex.h
	gcc -Wall -std=c99 -c comment_tex.c

comment_makefile.o: comment_makefile.c comment_makefile.h
	comment comment_makefile.c comment_makefile.h
	gcc -Wall -std=c99 -c comment_makefile.c

$(OBJECTS): comment.h

comment.h:
	comment comment.h

.PHONY: install
install: comment
	cp -p ./comment ~/bin/comment

.PHONY: uninstall
uninstall:
	rm -f ~/bin/comment

.PHONY: clean
clean:
	rm -f *.o comment

