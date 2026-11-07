# Builtin error details, as bash 5.3: "too many arguments" abandons the
# text (status 2 in a script, 1 for -c, eval or a comsub) and return
# checks its count first; alias, unalias, readonly and builtin reject
# unknown options; declare -f refuses attributes a function cannot
# have, assignments and lifting readonly, and prints a named function's
# body alone; hash without hashing, and -d or -t with no name; cd with
# a readonly PWD or OLDPWD fails though the directory changes; the call
# stack arrays cannot be unset; select's bad name cites the line bash
# last set; a readonly function's redefinition cites the parser's line.
exec 2>&1
printf 'shift 1 2; echo same\necho next $?\nf() { return 1 2; }\nf\necho next $?\nreturn 1 2\necho next $?\n' > s1
bash s1
bash -c 'shift 1 2'; echo "st $?"
bash -c 'x=$(exit 1 2); echo "cs $?"; (exit 1 2); echo "sub $?"'
alias -x a=b; echo $?
alias -px; echo $?
unalias -x a; echo $?
unalias -ax; echo $?
unalias --x; echo $?
alias a=b; alias -p a; unalias a
readonly -x v; echo $?
readonly -i v; echo $?
readonly -n v; v=1; echo "v=$v $?"
declare -a arr; readonly -n arr; echo $?
builtin -x; echo $?
builtin -- echo builtin
f() { :; }
declare -f -a f; echo $?
declare -f -ia f; echo $?
declare -f -nA f; echo $?
declare -f -a; echo $?
declare -f +a f; echo $?
declare -f x=1; echo $?
declare -fp x=1; echo $?
declare -fx f; declare -f f; declare -fp f
readonly -f f; declare -f +r f; echo $?
declare -F +r f; echo $?
hash -d; echo $?
hash -t; echo $?
set +h; hash; echo $?; hash -r; echo $?; set -h
mkdir d
bash -c 'readonly PWD; cd d; echo "$? ${PWD##*/}"; pwd | sed "s|.*/||"'
bash -c 'readonly OLDPWD; cd d; echo "$? ${PWD##*/}"'
for v in BASH_ARGC BASH_ARGV BASH_LINENO BASH_SOURCE FUNCNAME; do bash -c "unset $v; echo $v \$?"; done
bash -o posix -c 'unset BASH_SOURCE; echo after'; echo "st $?"
printf 'g()\n{\n\t:\n\n\tselect $1 in a; do :; done\n}\ng "a b"\nfor i in 1; do\n\n\tselect $1 in a; do :; done\ndone\nwhile :; do\n\n\tselect $1 in a; do :; done; break\ndone\n' > s2
bash s2 </dev/null
printf 'h() { :; }\nreadonly -f h\nh()\n{\n\techo bar\n}\n' > s3
bash s3
