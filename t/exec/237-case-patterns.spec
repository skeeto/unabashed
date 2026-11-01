== stdout 208
a -> [ab]
b -> [ab]
x -> default
z -> [!a-y]
ab -> ?b
abc -> a*c
a.c -> a*c
A -> [!a-y]
- -> [!a-y]
! -> [!a-y]
literal backslash
lone bracket quoted
lone bracket escaped
escaped star prefix
bracket question

== stderr 0

== merged 208
a -> [ab]
b -> [ab]
x -> default
z -> [!a-y]
ab -> ?b
abc -> a*c
a.c -> a*c
A -> [!a-y]
- -> [!a-y]
! -> [!a-y]
literal backslash
lone bracket quoted
lone bracket escaped
escaped star prefix
bracket question

== status 0
