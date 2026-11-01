# eval joins its arguments with spaces and runs the result
eval echo hello world
eval 'echo "quoted  spaces"'
eval echo '"a  b"'
cmd='x=5; echo $x'
eval "$cmd"
echo "x=$x"
eval "y=1" "z=2"
echo "$y $z"
eval
echo "empty eval: $?"
eval ''
echo "empty string: $?"
eval '   '
echo "blank: $?"
eval false
echo "false: $?"
eval 'false;' true
echo "multi: $?"
eval "f() { echo from-eval-func; }"
f
