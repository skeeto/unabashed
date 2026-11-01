== stdout 383
*.@(c|h): a.c b.c c.h
!(*.c): Makefile README a.cc c.h d.o
+([a-d]).c: a.c b.c
?(a).c: a.c
*(a|b).c: a.c b.c
@(*.o|M*): Makefile d.o
.!(|.): .e.c .f
@(.*|*): .e.c .f Makefile README a.c a.cc b.c c.h d.o
!(*.?): Makefile README a.cc
*.!(c|h): a.cc d.o
@(a|b).c?(c): a.c a.cc b.c
R!(x): README
dot !(*.c): .f Makefile README a.cc c.h d.o
dot *.c: .e.c a.c b.c
null +(z): 
off: a.c b.c

== stderr 0

== merged 383
*.@(c|h): a.c b.c c.h
!(*.c): Makefile README a.cc c.h d.o
+([a-d]).c: a.c b.c
?(a).c: a.c
*(a|b).c: a.c b.c
@(*.o|M*): Makefile d.o
.!(|.): .e.c .f
@(.*|*): .e.c .f Makefile README a.c a.cc b.c c.h d.o
!(*.?): Makefile README a.cc
*.!(c|h): a.cc d.o
@(a|b).c?(c): a.c a.cc b.c
R!(x): README
dot !(*.c): .f Makefile README a.cc c.h d.o
dot *.c: .e.c a.c b.c
null +(z): 
off: a.c b.c

== status 0
