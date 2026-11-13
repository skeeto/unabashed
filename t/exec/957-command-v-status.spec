== stdout 168
echo
for
f
v some 0
v none 1
echo
v last 0
echo is a shell builtin
f is a function
f () 
{ 
    :
}
V some 0
V none 1
echo
pv some 0
type some 1
keyword
type -t some 1

== stderr 166
bash: line 9: command: nosuch: not found
bash: line 10: command: nosuch1: not found
bash: line 10: command: nosuch2: not found
bash: line 12: type: nosuch: not found

== merged 334
echo
for
f
v some 0
v none 1
echo
v last 0
bash: line 9: command: nosuch: not found
echo is a shell builtin
f is a function
f () 
{ 
    :
}
V some 0
bash: line 10: command: nosuch1: not found
bash: line 10: command: nosuch2: not found
V none 1
echo
pv some 0
bash: line 12: type: nosuch: not found
type some 1
keyword
type -t some 1

== status 0
