# eval re-parses expanded text: double expansion and quoting
v='$HOME'
eval echo $v
eval echo "'$v'"
eval "echo \$v"
name=target
eval "$name=assigned"
echo "$target"
eval "echo \"\$$name\""
a='b c'
eval "set -- $a"
echo "$#"
eval "set -- \"$a\""
echo "$#"
eval 'echo $(echo nested)'
eval "echo \`echo bq\`"
eval 'echo "a;b"; echo c'
eval echo \"semi\;colon\"
s="it's"
eval "echo \"$s\""
