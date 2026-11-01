# Invalid regular expressions: status 2 and glibc's messages
for re in '(' 'a)' '[' '[a' 'a{1' 'a{2,1}' '*a' '\' '(a)\2' '[[:foo:]]' \
    '[z-a]' '[[.xx.]]' 'a{99999}' '[[=ab=]]' 'a{,1,2}' '(a|*b)' '^*' \
    'a{}' 'a{x}' '[a-[:digit:]]' '[[:alpha:]-z]' '(a)|\1' '[[.'; do
    [[ abc =~ $re ]]
    echo "$? <$re>"
done
re='('
[[ ! x =~ $re ]]; echo "! $?"
[[ x =~ $re || x =~ x ]]; echo "|| $?"
[[ x =~ $re && x =~ x ]]; echo "&& $?"
if [[ x =~ $re ]]; then echo yes; else echo "else $?"; fi
declare -p BASH_REMATCH 2>&1
set -x
[[ a.b =~ a\.b ]]
[[ abc =~ "a"b. ]]
re='x|(y)'; [[ zy =~ $re ]]
[[ a =~ ["]"] ]]
set +x
[[ -t X ]]; echo "t $?"
[[ ! -t X ]]; echo "!t $?"
[[ -t X || 1 ]]; echo "t|| $?"
[[ 1 -eq X ]]; echo "eq $?"
echo done
