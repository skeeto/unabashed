# In a [[ ]] arithmetic operand (and -v's), bash expands the word with
# Q_ARITH: a double-quoted part's \" loses its backslash as the text
# is extracted and is then read as a quote, which goes. So \" in
# double quotes vanishes there, and only there: a single-quoted ", a "
# from an expansion, \" outside double quotes or in ${...} stay, and
# the string comparisons are as ever.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
done <<'EOF'
[[ 3 -ge "\"" ]]; echo $?
[[ "\"3" -eq 3 ]]; echo $?
[[ 3 -eq "\"3\"" ]]; echo $?
[[ "1\"+2" -eq 3 ]]; echo $?
ab=4; [[ "a\"b" -eq 4 ]]; echo $?
[[ 3 -eq "\\\"3" ]]; echo $?
x=1; [[ -v "\"x" ]]; echo $?
set -x; [[ "\"3" -lt 4 ]]; echo $?
[[ 3 -ge '"' ]]; echo $?
x='"'; [[ 3 -ge "$x" ]]; echo $?
[[ \"3 -eq 3 ]]; echo $?
[[ 3 -eq "${u:-\"3}" ]]; echo $?
[[ 3 -eq "$(echo \"3)" ]]; echo $?
[[ "\"" == '"' ]]; echo $?
[[ -n "\"" ]]; echo $?
echo $(( "\"3" ))
EOF
