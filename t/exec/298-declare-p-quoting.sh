a='plain'
b='with space'
c="dq\"inside"
d='dollar $HOME and `tick`'
e='back\slash'
f="new
line"
g="tab	tab"
h="single'quote"
i=''
j='!bang'
k='~tilde'
declare -p a b c d e f g h i j k
declare -x x1='a"b'
declare -p x1
