== stdout 2955
-- declare -n _=foo; foo=1 echo a b; declare -p foo
a b
declare -- foo="_=foo"
-- declare -n _=foo; foo=0; declare -p foo; foo=1 true w; declare -p foo
declare -- foo=""
declare -- foo="foo"
-- declare -n _=foo; foo=1 bar=2 command echo u; declare -p foo
u
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 builtin echo t; declare -p foo
t
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 declare s; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 printf -v q x; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 let q=1; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 env true y; declare -p foo
declare -- foo="y"
-- declare -n _=foo; f() { echo "in [$foo]"; }; foo=1 f z; declare -p foo
in [1]
declare -- foo="z"
-- declare -n _=foo; foo=1 eval 'echo "[$foo]"' v; declare -p foo
[1] v
declare -- foo="v"
-- declare -n _=foo; foo=1 . /dev/null s; declare -p foo
declare -- foo="s"
-- declare -n _=foo; foo=1 read q </dev/null; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 command read q </dev/null; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 builtin read q </dev/null; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 unset q; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 mapfile q </dev/null; declare -p foo
declare -- foo="q"
-- set -o posix; declare -n _=foo; foo=1 : p; declare -p foo
declare -- foo="p"
-- declare -n _=foo; f() { foo=1 echo h; declare -p foo; }; f; declare -p foo
h
declare -- foo="_=foo"
declare -- foo="f"
-- f() { declare -n _=foo; foo=1 echo i; declare -p foo; }; f; declare -p foo
i
declare -- foo="i"
declare -- foo="foo"
-- declare -n _=foo; (foo=1 echo m; declare -p foo)
m
declare -- foo="_=foo"
-- declare -n _=foo; { foo=1 echo o; declare -p foo; } | cat
o
declare -- foo="_=foo"
-- trap 'echo "err [$_]"' ERR; : a; false x; f() { return 1; }; f y; nosuch q 2>/dev/null; x=$(false)
err [x]
err [y]
err [q]
err []
-- set -e; trap 'echo "exit [$_]"' EXIT; false q
exit [q]
-- : 5; declare -i _; echo '1+'; echo after
1+
bash: line 1: echo: 1+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; x=1 true '1/0'
bash: line 1: true: 1/0: division by 0 (error token is "0")
-- : 5; declare -i _; command echo '6+'; echo after
6+
bash: line 1: echo: 6+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; f() { :; }; f '1/0'
bash: line 1: :: :: arithmetic syntax error: operand expected (error token is ":")
-- : 5; declare -i _; f() { echo '4+'; }; f; echo after
4+
bash: line 1: echo: 4+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; nosuchcmd '2+' 2>/dev/null; echo after
bash: line 1: nosuchcmd: 2+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; read q <<<'7+'; echo after
bash: line 1: read: 7+: arithmetic syntax error: operand expected (error token is "+")

== stderr 0

== merged 2955
-- declare -n _=foo; foo=1 echo a b; declare -p foo
a b
declare -- foo="_=foo"
-- declare -n _=foo; foo=0; declare -p foo; foo=1 true w; declare -p foo
declare -- foo=""
declare -- foo="foo"
-- declare -n _=foo; foo=1 bar=2 command echo u; declare -p foo
u
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 builtin echo t; declare -p foo
t
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 declare s; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 printf -v q x; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 let q=1; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 env true y; declare -p foo
declare -- foo="y"
-- declare -n _=foo; f() { echo "in [$foo]"; }; foo=1 f z; declare -p foo
in [1]
declare -- foo="z"
-- declare -n _=foo; foo=1 eval 'echo "[$foo]"' v; declare -p foo
[1] v
declare -- foo="v"
-- declare -n _=foo; foo=1 . /dev/null s; declare -p foo
declare -- foo="s"
-- declare -n _=foo; foo=1 read q </dev/null; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 command read q </dev/null; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 builtin read q </dev/null; declare -p foo
declare -- foo="_=foo"
-- declare -n _=foo; foo=1 unset q; declare -p foo
declare -- foo="q"
-- declare -n _=foo; foo=1 mapfile q </dev/null; declare -p foo
declare -- foo="q"
-- set -o posix; declare -n _=foo; foo=1 : p; declare -p foo
declare -- foo="p"
-- declare -n _=foo; f() { foo=1 echo h; declare -p foo; }; f; declare -p foo
h
declare -- foo="_=foo"
declare -- foo="f"
-- f() { declare -n _=foo; foo=1 echo i; declare -p foo; }; f; declare -p foo
i
declare -- foo="i"
declare -- foo="foo"
-- declare -n _=foo; (foo=1 echo m; declare -p foo)
m
declare -- foo="_=foo"
-- declare -n _=foo; { foo=1 echo o; declare -p foo; } | cat
o
declare -- foo="_=foo"
-- trap 'echo "err [$_]"' ERR; : a; false x; f() { return 1; }; f y; nosuch q 2>/dev/null; x=$(false)
err [x]
err [y]
err [q]
err []
-- set -e; trap 'echo "exit [$_]"' EXIT; false q
exit [q]
-- : 5; declare -i _; echo '1+'; echo after
1+
bash: line 1: echo: 1+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; x=1 true '1/0'
bash: line 1: true: 1/0: division by 0 (error token is "0")
-- : 5; declare -i _; command echo '6+'; echo after
6+
bash: line 1: echo: 6+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; f() { :; }; f '1/0'
bash: line 1: :: :: arithmetic syntax error: operand expected (error token is ":")
-- : 5; declare -i _; f() { echo '4+'; }; f; echo after
4+
bash: line 1: echo: 4+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; nosuchcmd '2+' 2>/dev/null; echo after
bash: line 1: nosuchcmd: 2+: arithmetic syntax error: operand expected (error token is "+")
-- : 5; declare -i _; read q <<<'7+'; echo after
bash: line 1: read: 7+: arithmetic syntax error: operand expected (error token is "+")

== status 1
