comment: comment.o comment_c.o comment_sh.o comment_tex.o comment_makefile.o
	gcc -o comment comment.o comment_c.o comment_sh.o comment_tex.o comment_makefile.o

comment.o: comment.c
	gcc -Wall -std=c99 -c comment.c

comment_c.o: comment_c.c
	gcc -Wall -std=c99 -c comment_c.c

comment_sh.o: comment_sh.c
	gcc -Wall -std=c99 -c comment_sh.c

comment_tex.o: comment_tex.c
	gcc -Wall -std=c99 -c comment_tex.c

comment_makefile.o: comment_makefile.c
	gcc -Wall -std=c99 -c comment_makefile.c

