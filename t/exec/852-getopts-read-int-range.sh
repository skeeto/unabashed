set -- -a -b -c
for i in 9223372036854775807 99999999999999999999 -9223372036854775809 2147483649 4294967297 4294967298 4294967299 0 -1; do OPTIND=$i; getopts abc o; echo "[$i] st=$? $o $OPTIND"; done
read -n 2147483648 v <<< abc; echo st=$?
read -N 4294967297 v <<< abc; echo st=$?
read -n 2147483647 v <<< abc; echo st=$? $v
read -u 4294967296 v; echo st=$?
read -u 2147483647 v; echo st=$?
