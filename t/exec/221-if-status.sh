# exit status of if in edge cases
if false; then echo no; fi
echo "false cond, no else: $?"
if true; then false; fi
echo "body false: $?"
if true; then (exit 7); fi
echo "body 7: $?"
if (exit 3); then echo no; else echo "else sees $?"; fi
if (exit 4); then :; elif echo "elif sees $?"; (exit 5); then :; fi
echo "after failed elif: $?"
if (exit 6); then :; elif (exit 7); then :; else echo "else sees $?"; fi
if ! (exit 3); then echo "negated sees $?"; fi
if true; then :; fi
echo "colon body: $?"
false
if true; then :; else :; fi
echo "reset: $?"
if (exit 2); then :; else (exit 9); fi
echo "else status: $?"
