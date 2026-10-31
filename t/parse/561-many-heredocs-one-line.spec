== stdout 149
f () 
{ 
    c <<1 <<2 3<<3 4<<-4 <<'5' <<'6' 7<<'7' 8<<8 9<<9 10<<10
one
1
two
2
three
3
four
4
five $x
5
six $x
6
seven
7
eight
8
nine
9
ten
10

}

== stderr 0

== merged 149
f () 
{ 
    c <<1 <<2 3<<3 4<<-4 <<'5' <<'6' 7<<'7' 8<<8 9<<9 10<<10
one
1
two
2
three
3
four
4
five $x
5
six $x
6
seven
7
eight
8
nine
9
ten
10

}

== status 0
