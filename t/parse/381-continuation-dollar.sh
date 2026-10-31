f() {
    echo $\
(echo a) $\
{x} $\
((1+2)) a$\
b $\
$ "$\
{y}"
}
declare -f
