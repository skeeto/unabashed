# BASH_COMPAT takes two digits, with or without a dot between, from 31
# to 53, or nothing; any other value warns as it is assigned, and again
# wherever a binding of it is undone onto such a value.
for v in 3.1 31 5.3 53 5.0 '' 3.0 30 54 5.4 4 4.2.1 042 a1 .4 ' 42'; do
    echo "[$v]"
    BASH_COMPAT=$v
done
unset BASH_COMPAT
echo builtin; BASH_COMPAT=x true
echo function; f() { :; }; BASH_COMPAT=y f
echo program; BASH_COMPAT=z cat </dev/null
BASH_COMPAT=pv
echo twice; BASH_COMPAT=x true
echo restored; BASH_COMPAT=x cat </dev/null
echo expanded; : "$BASH_COMPAT"
unset BASH_COMPAT
echo export; BASH_COMPAT=w export BASH_COMPAT; declare -p BASH_COMPAT
echo others; BASH_COMPAT+=r; read BASH_COMPAT <<<rd; printf -v BASH_COMPAT %s pv
g() { local BASH_COMPAT=u; }; BASH_COMPAT=44; g; echo local
unset BASH_COMPAT
# a new shell checks the one it inherits once $0 is set
BASH_COMPAT=v bash -c 'echo child'
BASH_COMPAT=v BASH_ARGV0=q bash -c 'echo "child $0"'
export BASH_COMPAT=4.0; bash -c 'echo quiet'
