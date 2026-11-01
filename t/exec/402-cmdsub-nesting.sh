# Nested command substitutions
echo $(echo $(echo $(echo deep)))
echo "$(echo "$(echo "in quotes")")"
echo `echo \`echo inner\``
echo $(echo `echo mixed`)
echo `echo $(echo mixed2)`
a=$(echo "x $(echo "y $(echo z)")")
echo "$a"
echo "$(echo ")")"
echo "$(echo '(' )"
echo $( echo "a)b" )
echo $(case x in x) echo case-ok;; esac)
echo $(case x in (x) echo case-paren;; esac)
