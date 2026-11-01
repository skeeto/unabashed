# extglob must be on when a line is parsed for its operators to parse;
# turned on inside the same line, it applies to later lines only.
shopt -u extglob
cd /tmp && mkdir xs && cd xs
touch a.c b.h
shopt -s extglob; echo after on
echo *.@(c|h)
f() { echo *.!(h); }
shopt -u extglob
f
x='*.+(c)'
echo $x
shopt -s extglob
echo $x
eval 'echo *.?(c)'
