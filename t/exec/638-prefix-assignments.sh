# Assignments before a command: += appends to (or adds under -i) the
# current value, and each one sees those before it
a=14
a+=5 printenv a
a+=5 eval 'echo $a'
f() { echo "f: $a"; }
a+=6 f
echo $a
declare -i n=3
n+=4 eval 'echo $n'
export e=1
e+=2 printenv e
K=dvb0.net A=${K#dvb} printenv A
K=1 A=$K eval 'echo $A'
g() { echo "g: $A"; }
K=3 A=x$K g
K=4 K=${K}x eval 'echo $K'
K=5 A=${K#dvb} echo "$A"
