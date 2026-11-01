== stdout 716
!()            a b aa ab abc ba bb foo foo.c foo.h .c c
!(*)          
!(a)*          '' a b aa ab abc ba bb foo foo.c foo.h .c c
!(a)b          b bb
a!(b)          a aa abc
!(a|b)         '' aa ab abc ba bb foo foo.c foo.h .c c
!(?)           '' aa ab abc ba bb foo foo.c foo.h .c
!(??)          '' a b abc foo foo.c foo.h c
!(*.c|*.h)     '' a b aa ab abc ba bb foo c
foo!(.c)       foo foo.h
!(foo).c       .c
!(!(a|b))      a b
*!(b)          '' a aa ab abc ba bb foo foo.c foo.h .c c
!(b)*          '' a b aa ab abc ba bb foo foo.c foo.h .c c
@(!(a)|a)      '' a b aa ab abc ba bb foo foo.c foo.h .c c
!(*(a))        b ab abc ba bb foo foo.c foo.h .c c
!(+(a|b))      '' abc foo foo.c foo.h .c c
neg ok
neg ok2

== stderr 0

== merged 716
!()            a b aa ab abc ba bb foo foo.c foo.h .c c
!(*)          
!(a)*          '' a b aa ab abc ba bb foo foo.c foo.h .c c
!(a)b          b bb
a!(b)          a aa abc
!(a|b)         '' aa ab abc ba bb foo foo.c foo.h .c c
!(?)           '' aa ab abc ba bb foo foo.c foo.h .c
!(??)          '' a b abc foo foo.c foo.h c
!(*.c|*.h)     '' a b aa ab abc ba bb foo c
foo!(.c)       foo foo.h
!(foo).c       .c
!(!(a|b))      a b
*!(b)          '' a aa ab abc ba bb foo foo.c foo.h .c c
!(b)*          '' a b aa ab abc ba bb foo foo.c foo.h .c c
@(!(a)|a)      '' a b aa ab abc ba bb foo foo.c foo.h .c c
!(*(a))        b ab abc ba bb foo foo.c foo.h .c c
!(+(a|b))      '' abc foo foo.c foo.h .c c
neg ok
neg ok2

== status 0
