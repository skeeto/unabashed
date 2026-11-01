# BASH_SUBSHELL counts nesting levels
echo "top $BASH_SUBSHELL"
(echo "one $BASH_SUBSHELL")
( (echo "two $BASH_SUBSHELL") )
echo "cs $(echo $BASH_SUBSHELL)"
echo "cs2 $(echo $(echo $BASH_SUBSHELL))"
(echo "mix $(echo $BASH_SUBSHELL)")
x=`echo $BASH_SUBSHELL`; echo "bq $x"
echo "after $BASH_SUBSHELL"
f() { echo "func $BASH_SUBSHELL"; }
f
(f)
{ echo "group $BASH_SUBSHELL"; }
