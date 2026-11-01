# Backquotes inside double quotes: escaped quotes and dollars
echo "`echo \"a b\"`"
echo "`echo '\"'`"
echo "x`echo \\`y"
echo "`echo \$HOME`"
v=1
echo "`echo \\$v`"
echo "`echo "inner"`"
echo `echo "a\`b"` 2>&1
