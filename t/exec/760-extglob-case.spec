== stdout 703
?(a)           '' a
*(a)           '' a aa
+(a)           a aa
@(a|b)         a b
!(a)           '' b ab ba aa aab abab abc c x.c x.h .c xc
+(a|b)         a b ab ba aa aab abab
*(ab)          '' ab abab
+(a)b          ab aab
a*(b)          a ab
!(a*)          '' b ba c x.c x.h .c xc
*.!(c)         x.h
*.@(c|h)       x.c x.h .c
@(|a)          '' a
?(|b)a         a ba
+(?(a)b)       b ab abab
*(+(a)b)       '' ab aab abab
!(!(a))        a
@(a|!(b))      '' a ab ba aa aab abab abc c x.c x.h .c xc
!(+(a)|b)      '' ab ba aab abab abc c x.c x.h .c xc
@(x.@(c|h))    x.c x.h
+(*.c|b)       b x.c .c
a!(b)*         a ab aa aab abab abc
literal parens
literal bar
two groups
from variable
quoted no match

== stderr 0

== merged 703
?(a)           '' a
*(a)           '' a aa
+(a)           a aa
@(a|b)         a b
!(a)           '' b ab ba aa aab abab abc c x.c x.h .c xc
+(a|b)         a b ab ba aa aab abab
*(ab)          '' ab abab
+(a)b          ab aab
a*(b)          a ab
!(a*)          '' b ba c x.c x.h .c xc
*.!(c)         x.h
*.@(c|h)       x.c x.h .c
@(|a)          '' a
?(|b)a         a ba
+(?(a)b)       b ab abab
*(+(a)b)       '' ab aab abab
!(!(a))        a
@(a|!(b))      '' a ab ba aa aab abab abc c x.c x.h .c xc
!(+(a)|b)      '' ab ba aab abab abc c x.c x.h .c xc
@(x.@(c|h))    x.c x.h
+(*.c|b)       b x.c .c
a!(b)*         a ab aa aab abab abc
literal parens
literal bar
two groups
from variable
quoted no match

== status 0
