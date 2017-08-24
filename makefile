comment: comment.o comment_c.o comment_sh.o comment_tex.o comment_makefile.o
	gcc -o comment comment.o comment_c.o comment_sh.o comment_tex.o comment_makefile.o

comment.o: comment.c $(wildcard *.h)
	gcc -Wall -std=c99 -c comment.c

comment_c.o: comment_c.c comment_c.h
	gcc -Wall -std=c99 -c comment_c.c

comment_sh.o: comment_sh.c comment_sh.h
	gcc -Wall -std=c99 -c comment_sh.c

comment_tex.o: comment_tex.c comment_tex.h
	gcc -Wall -std=c99 -c comment_tex.c

comment_makefile.o: comment_makefile.c comment_makefile.h
	gcc -Wall -std=c99 -c comment_makefile.c

$(patsubst %.c,%.o,$(wildcard *.c)): comment.h

install: comment
	cp -p ./comment ~/bin/comment

uninstall:
	rm -f ~/bin/comment

clean:
	rm -f *.o comment
