# { } runs in the current shell, ( ) does not
x=0
{ x=1; }
echo "group: $x"
(x=2)
echo "subshell: $x"
{ cd /tmp; }
echo "$PWD"
cd /home/oracle
{ (x=3); x=4; }
echo "$x"
( { x=5; }; echo "inner $x" )
echo "$x"
echo hi | { read v; echo "pipe group $v"; }
echo "v=${v-unset}"
