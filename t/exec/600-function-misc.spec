== stdout 257
function
this is zf
a
b
1
1
gone
f2 is a function
f2 () 
{ 
    echo a;
    { 
        echo b
    } 1>&2
} 2>&1
function a=2 () 
{ 
    :
}
11111 () 
{ 
    :
}
in 20
out 20
st=2
h0
h1
0
1
ab
<x>
<y>
a
c
b
bc
if
then
else
.
:
[
BASH_VERSINFO
BASH_VERSION
2

== stderr 292
bash: line 14: declare: `11111': not a valid identifier
bash: line 21: `break': is a special builtin
bash: line 24: `'a b'': not a valid identifier
bash: line 28: `'a b'': not a valid identifier
bash: line 31: `'c d'': not a valid identifier
bash: line 41: compgen: nope: invalid action name

== merged 549
function
this is zf
a
b
1
1
gone
f2 is a function
f2 () 
{ 
    echo a;
    { 
        echo b
    } 1>&2
} 2>&1
function a=2 () 
{ 
    :
}
11111 () 
{ 
    :
}
bash: line 14: declare: `11111': not a valid identifier
in 20
out 20
bash: line 21: `break': is a special builtin
st=2
bash: line 24: `'a b'': not a valid identifier
bash: line 28: `'a b'': not a valid identifier
bash: line 31: `'c d'': not a valid identifier
h0
h1
0
1
ab
<x>
<y>
a
c
b
bc
if
then
else
.
:
[
BASH_VERSINFO
BASH_VERSION
bash: line 41: compgen: nope: invalid action name
2

== status 0
