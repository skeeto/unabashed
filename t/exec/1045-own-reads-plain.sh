# The shell reads its own variables as bash's get_string_value does: a
# nameref to an array element reads as unset, and its subscript, here a
# command substitution, never runs
exec 2>&1
mkdir -p w/d/e; cd w; h=$PWD; a=(/ 1 x); r='a[$(echo ran >&2)1]'
(touch .x y; declare -n GLOBIGNORE=$r; echo *)
(declare -n PS4=$r; set -x; : t; set +x)
(declare -n TIMEFORMAT=$r; time :) 2>&1 | wc -l
(declare -n HOME=$r; cd; echo cd=$? ${PWD#$h})
(declare -n CDPATH=$r; cd e; echo cdpath $?)
(declare -n OLDPWD=$r; cd -; echo $?)
(declare -n TMOUT=$r; read -r x <<<hi; echo $x)
(f() { :; }; declare -n FUNCNEST=$r; f; echo fn)
(declare -n OPTERR=$r; getopts a o -b; echo "$o")
(declare -n PS3=$r; select x in a; do echo $x; break; done <<<1)
(declare -n COLUMNS=$r; select x in a b; do echo $x; break; done <<<1)
(HISTSIZE=5 HISTFILESIZE=5; set -o history; declare -n HISTFILESIZE=$r; echo hist)
(declare -n HISTIGNORE=$r; set -o history; HISTSIZE=5; echo x)
(declare -n IFS=$r; x="a b"; echo $x; read p q <<<"c d"; echo $p)
(declare -n TMPDIR=$r; cat <<<here)
