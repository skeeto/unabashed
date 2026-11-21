# When bash binds $_. After a builtin, while its temporary bindings
# last, so that a global nameref _ assigns the temporary foo, which then
# goes away; not for eval and source, whose commands end them, nor for
# unset, mapfile and read (unless through builtin), nor through a
# function's own nameref _; after a function's and a program's are
# gone. Before ERR and set -e, even for assignments alone. An integer
# _'s error names the command, as bash's this_command_name: the one
# command runs, or the last simple command a function ran.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c" 2>&1
done <<'EOF'
declare -n _=foo; foo=1 echo a b; declare -p foo
declare -n _=foo; foo=0; declare -p foo; foo=1 true w; declare -p foo
declare -n _=foo; foo=1 bar=2 command echo u; declare -p foo
declare -n _=foo; foo=1 builtin echo t; declare -p foo
declare -n _=foo; foo=1 declare s; declare -p foo
declare -n _=foo; foo=1 printf -v q x; declare -p foo
declare -n _=foo; foo=1 let q=1; declare -p foo
declare -n _=foo; foo=1 env true y; declare -p foo
declare -n _=foo; f() { echo "in [$foo]"; }; foo=1 f z; declare -p foo
declare -n _=foo; foo=1 eval 'echo "[$foo]"' v; declare -p foo
declare -n _=foo; foo=1 . /dev/null s; declare -p foo
declare -n _=foo; foo=1 read q </dev/null; declare -p foo
declare -n _=foo; foo=1 command read q </dev/null; declare -p foo
declare -n _=foo; foo=1 builtin read q </dev/null; declare -p foo
declare -n _=foo; foo=1 unset q; declare -p foo
declare -n _=foo; foo=1 mapfile q </dev/null; declare -p foo
set -o posix; declare -n _=foo; foo=1 : p; declare -p foo
declare -n _=foo; f() { foo=1 echo h; declare -p foo; }; f; declare -p foo
f() { declare -n _=foo; foo=1 echo i; declare -p foo; }; f; declare -p foo
declare -n _=foo; (foo=1 echo m; declare -p foo)
declare -n _=foo; { foo=1 echo o; declare -p foo; } | cat
trap 'echo "err [$_]"' ERR; : a; false x; f() { return 1; }; f y; nosuch q 2>/dev/null; x=$(false)
set -e; trap 'echo "exit [$_]"' EXIT; false q
: 5; declare -i _; echo '1+'; echo after
: 5; declare -i _; x=1 true '1/0'
: 5; declare -i _; command echo '6+'; echo after
: 5; declare -i _; f() { :; }; f '1/0'
: 5; declare -i _; f() { echo '4+'; }; f; echo after
: 5; declare -i _; nosuchcmd '2+' 2>/dev/null; echo after
: 5; declare -i _; read q <<<'7+'; echo after
EOF
