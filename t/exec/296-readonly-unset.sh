readonly ro=value
unset ro
echo "status $? ro=$ro"
unset -v ro
echo "status $?"
unset ro ok
echo "status $?"
declare -r dr=1
declare +r dr
echo "plus r status $?"
declare -p ro dr
readonly -p | while read -r line; do case $line in *ro=*|*dr=*) echo "$line";; esac; done
readonly novalue
declare -p novalue
