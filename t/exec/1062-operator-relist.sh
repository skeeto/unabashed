# The pattern operators (# % / ^ , ~) read $@, $* or an array's elements
# again once their words are expanded, as bash does; a scalar, $N or one
# element keeps the value from before. An unset or empty parameter's
# words are not expanded at all.
r() { printf '<%s>' "$@"; echo; }
unset u
: "${u#${ echo side1; }}"
: "${u[@]#${ echo side2; }}"
set --
: "${@#${ echo side3; }}"
: "${*%${ echo side3b; }}"
a=()
: "${a[@]/x/${ echo side4; }}"
: "${u/x/${ echo side5; }}"
: "${u^${ echo side6; }}"
: "${a[@],${ echo side7; }}"
declare -A h=()
: "${h[@]#${ echo side8; }}"
echo ---
set -- one two three
r "${@/o/${ set -- q oo; echo 0; }}"
set -- one two three
r "${*/o/${ set -- q oo; echo 0; }}"
set -- one two three
r "${@#${ set -- q oo; echo o; }}"
set -- one two three
r "${@%%${ set -- q oo; echo o; }}"
set -- one two three
r "${@^${ set -- q oo; echo o; }}"
set -- one two three
r "${@,,${ set -- Q OO; echo O; }}"
set -- one two three
r "${@~${ set -- q oo; echo o; }}"
set -- one two three
r "${1/o/${ set -- q oo; echo 0; }}" "$@"
set -- one two three
r "${2#${ set -- q oo; echo t; }}" "$@"
set -- one two three
r "${@/o/${ set -- q oo; echo 0; }x}" "$@"
set -- one two three
r "${@/${ set -- q oo; echo o; }/0}" "$@"
set -- one two three
r ${@/o/${ set -- q oo; echo 0; }}
set -- one two three
x="${@/o/${ set -- q oo; echo 0; }}"; r "$x"
echo --- arrays
a=(one two three)
r "${a[@]/o/${ a=(q oo); echo 0; }}"
a=(one two three)
r "${a[*]/o/${ a=(q oo); echo 0; }}"
a=(one two three)
r "${a[@]#${ a=(q oo); echo o; }}"
a=(one two three)
r "${a[@]%${ a[5]=oo; echo e; }}"
a=(one two three)
r "${a[@]^^${ a[1]=xx; echo [a-z]; }}"
a=(one two three)
r "${a[1]/w/${ a[1]=qq; echo W; }}" "${a[@]}"
a=(one two three)
r "${a/o/${ a=(q oo); echo 0; }}" "${a[@]}"
a=(one two three)
r "${a[0]#${ a=(q oo); echo o; }}" "${a[@]}"
declare -A h=([k]=one)
r "${h[@]/o/${ h[j]=oo; echo 0; }}"
declare -A h=([k]=one)
r "${h[k]/o/${ h[k]=oo; echo 0; }}" "${h[@]}"
declare -A h=([k]=one)
r "${h[@]#${ h[k]=two; echo t; }}"
echo --- scalars
s=one
r "${s/o/${ s=two; echo 0; }}" "$s"
s=one
r "${s#${ s=two; echo t; }}" "$s"
s=one
r "${s[@]/o/${ s=oops; echo 0; }}" "$s"
s=one
r "${s^${ s=two; echo t; }}" "$s"
echo --- namerefs and indirection
a=(one two three)
declare -n n=a
r "${n[@]/o/${ a=(q oo); echo 0; }}"
a=(one two three)
r "${n/o/${ a=(q oo); echo 0; }}"
a=(one two three)
i='a[@]'
r "${!i/o/${ a=(q oo); echo 0; }}"
set -- one two three
i=@
r "${!i/o/${ set -- q oo; echo 0; }}"
set -- one two three
i=1
r "${!i/o/${ set -- q oo; echo 0; }}"
a=(one two three)
i='a[@]'
r "${!i/o/${ i=s; echo 0; }}"
declare -n m=s
s=one
r "${m/o/${ s=oo; echo 0; }}"
declare -n e='a[1]'
a=(one two three)
r "${e/w/${ a[1]=ww; echo 0; }}"
echo --- empty after
set -- one two three
r "${@/o/${ set --; echo 0; }}" "$#"
a=(one two three)
r "${a[@]/o/${ a=(); echo 0; }}" "${#a[@]}"
set -- one two three
r "${@/o/${ set -- '' ''; echo 0; }}" "$#"
echo --- counts
set -- one two
r "${@/o/${ set -- q oo pp; echo 0; }}"
set -- one two three
r "${@/#/${ set -- q; echo x; }}"
set -- one two three
r "${@/%/${ set -- q; echo x; }}"
set -- one two three
r "${@//${ set -- q; echo; }/x}"
echo --- substrings
# The end a negative offset counts from, and the last parameter a length
# reaches, are as the offset left them; an offset past the end leaves
# the length unexpanded; the list is read after both.
set -- abc def
r "${@:1:${ set -- g h i j; echo 3; }}"
set -- abc def
r "${@:${ set -- g h i j; echo 1; }:3}"
set -- abc def
r "${@:${ set -- g h i j; echo -1; }}"
set -- abc def
r "${@:${ set -- g h i j; echo -2; }:1}"
set -- abc def
r "${@: -1:${ set -- g h i j; echo 2; }}"
set -- abc def
r "${@:0:${ set -- g h i j; echo 9; }}"
set -- abc def ghi jkl
r "${@:1:${ set -- g; echo 3; }}"
set -- abc def ghi jkl
r "${@:3:${ set -- g; echo 3; }}"
set -- abc def
r "${*:1:${ set -- g h i j; echo 3; }}"
set -- a
r "${@:5:-1}" "${@: -5:-1}" "${@:3}" "${@:2}" "${@:3:${ echo side; }}"
a=(one two three)
r "${a[@]:${ a=(1 2 3 4 5); echo -1; }}"
a=(one two three)
r "${a[@]: -1:${ a=(1 2 3 4 5); echo 2; }}"
a=(one two three)
r "${a[@]:1:${ a=(1 2 3 4 5); echo 9; }}"
a=(one two three four five)
r "${a[@]:${ a=(1 2); echo -1; }}"
a=(one two three four five)
r "${a[@]: -2:${ a=(1 2); echo 1; }}"
a=(1 2 3); a[7]=8
r "${a[@]:8:-1}" "${a[@]: -9:-1}" "${a[@]: -8}" "${a[@]: -9}" "${a[@]:9:${ echo side; }}"
declare -A h=([a]=1)
r "${h[@]:${ h[b]=2; h[c]=3; echo -1; }}"
declare -A h=([a]=1 [b]=2 [c]=3)
r "${h[@]: -1:${ unset 'h[b]'; echo 2; }}"
declare -A h=([a]=1 [b]=2 [c]=3)
r "${h[@]:1}" "${h[@]:3}" "${h[@]: -4}" "${h[@]:4}" "${h[@]:0:1}" "${h[@]: -1}"
s=abcdef
r "${s:${ s=xyz; echo 1; }:2}" "$s"
set -- abcdef
r "${1:${ set -- xyz; echo 1; }:2}" "$1"
a=(one two three)
r "${a[1]:${ a[1]=TWO; echo 1; }}" "${a[1]}"
