# set quotes a value only where it would be special, and uses $'...'
# for unprintable characters
a=ab#cd b='#x' c=a=b d='x~' e='~x' f='a b' g='p=~' h='q:~r' i=$'a\001b' j=$'x\ny' k=
set | grep -a '^[a-k]='
