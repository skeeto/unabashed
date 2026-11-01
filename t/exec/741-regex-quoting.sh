# The right side of =~: quoted characters match literally
[[ a.c =~ a.c ]]; echo 1 $?
[[ abc =~ "a.c" ]]; echo 2 $?
[[ abc =~ a\.c ]]; echo 3 $?
[[ a.c =~ 'a.c' ]]; echo 4 $?
re='a.c'; [[ abc =~ $re ]]; echo 5 $?
[[ abc =~ "$re" ]]; echo 6 $?
re='^(a+)\.'; [[ aa.b =~ $re ]]; echo 7 $? "${BASH_REMATCH[1]}"
[[ 'a*' =~ a"*" ]]; echo 8 $? "${BASH_REMATCH[0]}"
[[ 'aaa' =~ a"*" ]]; echo 9 $? "${BASH_REMATCH[0]}"
[[ x =~ \x ]]; echo 10 $?
[[ x =~ \\x ]]; echo 11 $?
[[ '\x' =~ \\x ]]; echo 12 $?
bs='\'; [[ '\[' =~ ${bs}[ ]]; echo 13 $?
[[ a-b =~ a\-b ]]; echo 14 $?
[[ a-b =~ "a\-b" ]]; echo 15 $?
[[ b =~ ["^"a] ]]; echo 16 $?
[[ '^' =~ [a"^"] ]]; echo 17 $?
[[ ']' =~ ["]"a] ]]; echo 18 $?
[[ '.' =~ ["."] ]]; echo 19 $?
[[ x =~ [[:alpha:]] ]]; echo 20 $?
[[ : =~ ["[:alpha:]"] ]]; echo 21 $?
[[ '(' =~ "(" ]]; echo 22 $?
[[ 'a|b' =~ a"|"b ]]; echo 23 $?
[[ ab =~ (a)(b)\2 ]]; echo 24 $?
[[ abb =~ (a)(b)\2 ]]; echo 25 $? "${BASH_REMATCH[*]}"
re='(a)(b)\2'; [[ abb =~ $re ]]; echo 26 $? "${BASH_REMATCH[*]}"
[[ '$x' =~ '$x' ]]; echo 27 $?
x=yes; [[ yes =~ ^$x$ ]]; echo 28 $?
[[ '[a' =~ [a ]]; echo 29 $?
[[ a =~ a{1} ]]; echo 30 $?
[[ 'a{1}' =~ a"{1}" ]]; echo 31 $?
c=$'\001'; [[ $c =~ $c ]]; echo 32 $?
[[ $c =~ \\$c ]]; echo 33 $?
r="\\$c"; [[ $c =~ $r ]]; echo 34 $?
[[ $'\177' =~ $'\177' ]]; echo 35 $?
[[ $'a\nb' =~ a.b ]]; echo 36 $?
[[ ab =~ a" "?b ]]; echo 37 $?
re='[[:alpha:]'; [[ a: =~ $re"]" ]]; echo 38 $? "${BASH_REMATCH[0]}"
[[ 'x]' =~ [[:alpha:]"]" ]]; echo 39 $? "${BASH_REMATCH[0]}"
[[ '[.' =~ "["[.] ]]; echo 40 $? "${BASH_REMATCH[0]}"
[[ a1 =~ [a][0-9] ]]; echo 41 $?
v='a\-b'; [[ a-b =~ ${v} ]]; echo 42 $?
[[ a-b =~ a${bs}-b ]]; echo 43 $?
[[ a-b =~ a\\-b ]]; echo 44 $?
