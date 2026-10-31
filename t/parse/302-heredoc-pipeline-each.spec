== stdout 119
f () 
{ 
    a <<A |
1
A
  b <<B |
2
B
  c <<C
3
C

}
g () 
{ 
    a <<A |
1
A
  b <<B |
2
B
  c <<C
3
C

    echo z
}

== stderr 0

== merged 119
f () 
{ 
    a <<A |
1
A
  b <<B |
2
B
  c <<C
3
C

}
g () 
{ 
    a <<A |
1
A
  b <<B |
2
B
  c <<C
3
C

    echo z
}

== status 0
