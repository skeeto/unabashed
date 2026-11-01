hash
cat </dev/null; cat </dev/null; true
hash
type cat; type -t cat; type -p cat; type -P cat; command -v cat; command -V cat
hash
hash -t cat; hash -t nope; echo $?
hash -l
hash ls; hash -t ls ls
hash -d ls; hash
hash -d ls; echo $?
hash -p /nonexist/foo foo; foo; echo $?
type foo; command -v foo; type -a foo
hash -t foo
( hash -r; hash ); hash
PATH=$PATH; hash
cat </dev/null; hash -p /tmp/x echo; hash; echo hi
f() { :; }; hash f; echo $?; hash -p /a/b; echo $?
hash -p /bin/sh sh; hash -p /tmp/bash bash
hash
hash -r; hash nonexist; echo $?
hash -z; echo $?
mkdir d; printf 'echo ran e\n' > d/e; chmod +x d/e; cd d
PATH=; type -p e; type e; e; hash
unset PATH; type -p e >/dev/null && echo found in cwd; e; nonexist; echo $?
