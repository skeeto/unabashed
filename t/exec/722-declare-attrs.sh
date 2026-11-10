# declare attribute flags, listings and errors
declare -i i=3+4; declare -l l=ABC; declare -u u=abc; declare -c c=hELLO; declare -t t=1; declare -x x=1
declare -ilx ilx=5; declare -p i l u c t x ilx
declare -lu lu=Mixed; declare -p lu
declare -ul ul=Mixed; declare -p ul
declare -cu cu=mixed; declare -p cu
declare -i i; i+=2; declare -p i
declare +i i; i=a+b; declare -p i
declare -p -- i
declare -a | sed "s/${BASH_VERSINFO[5]}/HOST/"; declare -A; declare -ar | grep -v BASH
declare -i | grep -v -E 'BASHPID|RANDOM|SRANDOM|SECONDS|HISTCMD|OPTIND|EUID|PPID|UID|LINENO'
declare -x | grep -E ' (x|ilx)='
declare -t
declare +x x; declare -p x
typeset -p i l
declare -g gg=1; f() { declare -g gg=2; local lv=1; declare -p gg lv; declare -gp lv 2>&1; }; f; declare -p gg
local x 2>&1; echo $?
declare -z 2>&1; echo $?
declare -p nosuch 2>&1; echo $?
declare -pf 2>&1 | head -2
f2() { local -; set -e; local; }; f2; echo "$-"
f3() { local a=1 b; local -p; local -p a; local; }; f3
declare -r ro=1; declare ro=2; echo $?; declare -p ro
declare -n nr=ro; declare -p nr; echo "${nr@a} ${ro@a} ${!nr@a}"
