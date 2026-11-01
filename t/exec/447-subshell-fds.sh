# File descriptors opened in subshells are closed afterwards
(exec 3>/tmp/f3; echo via3 >&3)
echo test >&3 2>/dev/null || echo "fd 3 closed in parent"
while read l; do echo "f3: $l"; done < /tmp/f3
x=$(exec 4>/tmp/f4; echo x >&4; echo captured)
echo "$x"
echo test >&4 2>/dev/null || echo "fd 4 closed"
exec 5>/tmp/f5
(echo from-sub >&5)
echo parent >&5
exec 5>&-
while read l; do echo "f5: $l"; done < /tmp/f5
