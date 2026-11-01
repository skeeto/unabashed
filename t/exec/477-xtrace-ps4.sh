PS4='>> '
set -x
echo one
PS4='[$LINENO] '
echo two
PS4='+$x+ '
x=A
echo three
PS4=''
echo four
unset PS4
echo five
PS4='ab'
echo six
PS4='%% '
echo seven
x=1; echo "$x"
