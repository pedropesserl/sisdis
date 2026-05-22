all: lider

lider: chang-roberts.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic chang-roberts.o smpl.o rand.o -lm -ggdb

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

chang-roberts.o: t1/chang-roberts.c smpl.h
	$(COMPILE.c) -g  t1/chang-roberts.c -I.

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) */*.o *.o lider

