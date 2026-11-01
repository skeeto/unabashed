# ${ cmd; } runs in the current shell and captures stdout
x=${ echo hello; }
echo "[$x]"
a=1
y=${ a=2; echo $a; }
echo "$y $a"
z=${ printf 'trail\n\n\n'; }
echo "[$z]"
w=${ false; }
echo "status $?"
w=${ (exit 5); }
echo "status $?"
echo "in dq: ${ echo inside; }"
f() { echo func-out; return 3; }
v=${ f; }
echo "$v $?"
n=${ echo $BASH_SUBSHELL; }
echo "subshell level $n"
