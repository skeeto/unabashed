# Command substitution inside parameter expansion operators
unset u
echo "${u:-$(echo default)}"
echo "${u-$(echo "dq default")}"
echo ${u:=$(echo assigned)}
echo "$u"
set_v=val
echo "${set_v:+$(echo alt)}"
echo "${set_v:-$(echo notrun; exit 1)}"
p=abcabc
echo "${p#$(echo a)}"
echo "${p%$(echo bc)}"
echo "${p/$(echo b)/$(echo X)}"
echo "${p//$(echo b)/"$(echo Y)"}"
unset u2
echo "${u2:-"$(echo 'a  b')"}"
echo ${u2:-$(echo 'a  b')}
