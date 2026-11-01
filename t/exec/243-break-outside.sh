# break and continue outside any loop
break
echo "break: $?"
continue
echo "continue: $?"
false
break 2
echo "break 2: $?"
break abc
echo "break abc: $?"
continue 0
echo "continue 0: $?"
break 1 2
echo "break 1 2: $?"
f() { break; echo "in f: $?"; }
f
{ continue; echo "group: $?"; }
if true; then break; echo "if: $?"; fi
