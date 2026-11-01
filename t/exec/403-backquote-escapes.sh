# Backslash handling inside backquotes
echo `echo \$HOME`
echo `echo '\$x'`
echo `echo "\$HOME"`
echo "`echo \"dq\"`"
echo `echo a\\b`
echo `echo \\`
v=val
echo `echo $v`
echo `echo \$v`
echo "`echo '\'`"
echo `printf '%s\n' \`echo nest\``
echo `echo \a\b`
