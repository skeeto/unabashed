PS4='+ $(echo sub) '
set -x
echo one
set +x
PS4='+ ${undef-dflt} '
set -x
echo two
set +x
PS4='+${FOO:=init} '
set -x
echo three "$FOO"
set +x
PS4='+ $((1+1)) '
set -x
echo four
set +x
PS4='+ ${bad x} '
set -x
echo five
set +x
echo done
