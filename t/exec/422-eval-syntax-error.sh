# eval syntax errors: message, status 2, shell continues
eval 'if true; then'
echo "status $?"
eval 'echo ('
echo "status $?"
eval 'fi'
echo "status $?"
eval 'echo "unterminated'
echo "status $?"
eval 'echo ok; (('
echo "status $?"
eval 'case x in'
echo "status $?"
eval 'echo $(' 
echo "status $?"
x=$(eval 'done' 2>&1)
echo "captured: $x"
echo still alive
