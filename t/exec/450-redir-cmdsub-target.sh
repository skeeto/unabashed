# Command substitution in redirection targets
echo one > /tmp/$(echo t1)
while read l; do echo "$l"; done < "/tmp/$(echo t1)"
echo two > $(echo /tmp/a b) 2>&1
echo "st=$?"
echo three > "$(echo /tmp/a b)"
while read l; do echo "got $l"; done < "/tmp/a b"
echo four > $(:)
echo "st=$?"
