all: lider

lider: randomized_leader_election.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic randomized_leader_election.o smpl.o rand.o -lm -ggdb

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

chang-roberts.o: randomized_leader_election.c smpl.h
	$(COMPILE.c) -g  randomized_leader_election.c -I.

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) */*.o *.o lider

