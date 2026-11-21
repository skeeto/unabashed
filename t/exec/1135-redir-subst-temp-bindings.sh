# A command or process substitution in a redirection word runs without
# the command's temporary bindings, as bash flushes its temporary
# environment there, and sees the variable beneath, exported or not;
# eval's text and a function's own redirections see them as usual.
cd "$(mktemp -d)"
a=2; a=9 echo foo >&$(echo $a)
unset FOO
FOO=bar cat < <(echo "$FOO:1"; echo "$FOO:2")
FOO=global
FOO=bar cat < <(echo "$FOO:1"; echo "$FOO:2")
FOO=bar read x < <(echo "$FOO:3"); echo "x=$x FOO=$FOO"
export FOO
FOO=bar bash -c 'echo "env $FOO"' > "a$(bash -c 'echo $FOO').out"
cat aglobal.out
FOO=bar : > "b$(echo $FOO).out"
FOO=bar : > "c${ echo $FOO; }.out"
FOO=bar : > "d${ FOO=new; echo $FOO; }.out"; echo "FOO=$FOO"
a=2 eval 'echo baz >&$a' 2>&1
FOO=bar eval 'echo "eval $FOO" > "e$FOO.out"'; cat ebar.out
g() { echo "g $FOO"; } > "f$FOO.out"
FOO=in g; cat fin.out
h() { echo "h $FOO" > "h$FOO.out"; }
FOO=in h; cat hin.out
echo *.out
