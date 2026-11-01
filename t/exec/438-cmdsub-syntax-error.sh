# Syntax errors inside command substitution at runtime via eval
eval 'x=$(if)'
echo "st=$?"
eval 'echo $(echo ok; fi)'
echo "st=$?"
eval 'echo `fi`'
echo "st=$?"
y=`eval 'done' 2>&1`
echo "y=$y"
echo alive
