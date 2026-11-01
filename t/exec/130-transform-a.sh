# ${x@a} attribute letters
p() { printf '<%s>' "$@"; echo; }
plain=1; unset u
p "${plain@a}" "${u@a}"
declare -i int=5; p "${int@a}"
export ex=1; p "${ex@a}"
readonly ro=1; p "${ro@a}"
declare -l lo=ABC; p "${lo@a}" "$lo"
declare -u up=abc; p "${up@a}" "$up"
declare -t tr=1; p "${tr@a}"
declare -irx all=3; p "${all@a}"
declare -x novalue; p "${novalue@a}"
