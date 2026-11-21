# A simple command's redirections expand without its own temporary
# bindings, as bash's expanding_redir has variable lookup skip the
# temporary environment: a word there sees, and assigns, the variable
# beneath. The bindings are still made first, and what the command
# runs sees them: a builtin, a function, a program, exec, a stage.
cd "$(mktemp -d)"
a=42; a=2 echo foo 2>&1 >&$a; echo "a $?"
a=1; a=2 echo bar >&$a
unset FOO
FOO=bar : > "b$FOO.out"
f() { echo "f $FOO"; }
FOO=bar f > "c$FOO.out"; cat c.out
FOO=bar cat </dev/null > "d$FOO.out"
FOO=bar declare -a arr=(1 2) > "e$FOO.out"; echo "arr ${arr[*]}"
FOO=bar echo stage > "g$FOO.out" | cat
echo x | FOO=bar cat > "h$FOO.out"
FOO=bar exec 3> "i$FOO.out"; echo "FOO=${FOO-unset}"; exec 3>&-
(FOO=bar exec cat </dev/null > "j$FOO.out")
FOO=bar > "k$FOO.out"; echo "FOO=$FOO"; unset FOO
FOO=bar cat <<EOF
here [$FOO]
EOF
x=1; x=2 : > "l$((x+=4)).out"; echo "x=$x"
unset y; y=3 : > "m${y:=7}.out"; echo "y=$y"
v="a,b"; IFS=, : > $v; echo "ifs $?"
X=1 X+=2 : > "n$X.out"
FOO=g; FOO=t : > "o$FOO.out" 4> "p$FOO.out"
declare -n ref=FOO
ref=r : > "q$FOO$ref.out"; echo "FOO=$FOO"
unset -n ref
set -o posix
FOO=s : > "r$FOO.out"; echo "posix FOO=$FOO"
set +o posix
echo *.out
