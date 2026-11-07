# return checks its argument before anything else: a non-number is
# reported (status 2), and only then whether there is anything to
# return from; a second argument after a number abandons the command
# line (all of -c's text, even from eval: subshells hold it here), also
# from a mapfile callback
exec 2>&1
f() { return 3 0; echo "in f"; }
(f; echo "f=$?"); echo "fsub=$?"
g() { return x 0; echo "in g"; }; g; echo "g=$?"
h() { return x; echo "in h"; }; h; echo "h=$?"
return x 0; echo "topx=$?"
return x; echo "topx1=$?"
( return 3 0; echo "sub" ); echo "sub=$?"
( return x 0; echo "subx" ); echo "subx=$?"
printf 'a\nb\n' | mapfile -c 1 -C 'return 3 0 x' arr; echo "s=$?"
printf 'a\nb\n' | { mapfile -c 1 -C 'return 3 0 x' arr; echo "s=$?"; }; echo "grp=$?"
printf 'a\nb\n' | mapfile -c 1 -C 'return x' arr; echo "x=$?"
(eval 'mapfile -c 1 -C "return 3 0" arr < <(printf "a\nb\n"); echo cur'; echo "in sub"); echo "cur=$?"
return 3 0; echo "top=$?"
echo "not reached"
