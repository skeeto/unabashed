# exit inside ${ } exits the shell itself
echo start
x=${ echo captured; }
echo "$x"
y=${ cd /tmp; pwd; }
echo "$y $PWD"
z=${ exit 7; }
echo never
