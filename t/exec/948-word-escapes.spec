== stdout 371
10 <test\n> <a"b> <a\b> <a$b> <a`b> <a\'b> <x\y> <\> <\\> <\a\b\c>
12 <a b> <*> <?> <[> <~> <{a,b}> <a$b> <a\b> <"q"> <'s'> <#> <a;b>
6 <a b> <"x"> <a\nb> <x $ y> <it's> <\$>
6 <$HOME> <$HOME> <`echo hi`> <ab*> <a b*> <ab>
2 <ab> <ab>
6 <> <> < > <\ > <\ > <\\>
test\n
test
a\tb|atb
4 <$x> <\1> <$x> <\1>
3 <a${x}b> <a${x}b> <${x:-d}>
1 <al\ias>
cq
cu
t1
3 <\t> <\n> <n>

== stderr 0

== merged 371
10 <test\n> <a"b> <a\b> <a$b> <a`b> <a\'b> <x\y> <\> <\\> <\a\b\c>
12 <a b> <*> <?> <[> <~> <{a,b}> <a$b> <a\b> <"q"> <'s'> <#> <a;b>
6 <a b> <"x"> <a\nb> <x $ y> <it's> <\$>
6 <$HOME> <$HOME> <`echo hi`> <ab*> <a b*> <ab>
2 <ab> <ab>
6 <> <> < > <\ > <\ > <\\>
test\n
test
a\tb|atb
4 <$x> <\1> <$x> <\1>
3 <a${x}b> <a${x}b> <${x:-d}>
1 <al\ias>
cq
cu
t1
3 <\t> <\n> <n>

== status 0
