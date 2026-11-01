== stdout 327
*: ++ .hid ; a.c b.c sub x:y
sub/*: sub/s.c sub/t.o
*: ++ .hid ; c.o sub x:y
sub/*: sub/s.c
colon in bracket: ++ .hid ; a.c b.c c.o sub
two: ++ .hid ; c.o sub
extglob: .hid sub x:y
group: ++ .hid ; a.c b.c c.o
all: *
empty patterns: ++ .hid ; a.c b.c c.o sub x:y
dots: .* ++ ; a.c b.c c.o sub x:y
nocase: ++ .hid ; c.o sub x:y

== stderr 0

== merged 327
*: ++ .hid ; a.c b.c sub x:y
sub/*: sub/s.c sub/t.o
*: ++ .hid ; c.o sub x:y
sub/*: sub/s.c
colon in bracket: ++ .hid ; a.c b.c c.o sub
two: ++ .hid ; c.o sub
extglob: .hid sub x:y
group: ++ .hid ; a.c b.c c.o
all: *
empty patterns: ++ .hid ; a.c b.c c.o sub x:y
dots: .* ++ ; a.c b.c c.o sub x:y
nocase: ++ .hid ; c.o sub x:y

== status 0
