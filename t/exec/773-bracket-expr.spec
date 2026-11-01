== stdout 756
[abc]            a b
[a-c]            a b
[!a-y]           z A Z 0 9 - ] [ ! ^ _ .   \
[^a-y]           z A Z 0 9 - ] [ ! ^ _ .   \
[]a]             a ]
[!]a]            b z A Z 0 9 - [ ! ^ _ .   \ x
[a-]             a -
[-z]             z -
[[:alpha:]]      a b z A Z x
[[:digit:]_]     0 9 _
[[:punct:]]      - ] [ ! ^ _ . \
[[:space:][:upper:]] A Z  
[![:alnum:]]     - ] [ ! ^ _ .   \
[[:xdigit:]]     a b A 0 9
[\]]             ]
[\!]             !
[a\-z]           a z -
[[]              [
[\]             
[[.-.]]          -
[[=a=]]          a
[Z-a]            a Z ] [ ^ _ \
[a-a]            a
[z-a]           
[!]             
[a              
x[a              x[a
[[:bogus:]]     
[[:alpha:]      
[[:alpha:]]x     ax bx
[ab][cd]         ac bd ad

== stderr 0

== merged 756
[abc]            a b
[a-c]            a b
[!a-y]           z A Z 0 9 - ] [ ! ^ _ .   \
[^a-y]           z A Z 0 9 - ] [ ! ^ _ .   \
[]a]             a ]
[!]a]            b z A Z 0 9 - [ ! ^ _ .   \ x
[a-]             a -
[-z]             z -
[[:alpha:]]      a b z A Z x
[[:digit:]_]     0 9 _
[[:punct:]]      - ] [ ! ^ _ . \
[[:space:][:upper:]] A Z  
[![:alnum:]]     - ] [ ! ^ _ .   \
[[:xdigit:]]     a b A 0 9
[\]]             ]
[\!]             !
[a\-z]           a z -
[[]              [
[\]             
[[.-.]]          -
[[=a=]]          a
[Z-a]            a Z ] [ ^ _ \
[a-a]            a
[z-a]           
[!]             
[a              
x[a              x[a
[[:bogus:]]     
[[:alpha:]      
[[:alpha:]]x     ax bx
[ab][cd]         ac bd ad

== status 0
