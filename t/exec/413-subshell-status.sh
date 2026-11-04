# Subshell exit status and exit inside subshells
(exit 3)
echo "exit 3: $?"
(false)
echo "false: $?"
(true; false; true)
echo "last true: $?"
(exit 300)
echo "exit 300: $?"
(exit -1)
echo "exit -1: $?"
(echo a; exit 2; echo b)
echo "st=$?"
( (exit 5) ; echo "inner $?"; exit 6 )
echo "outer $?"
! (exit 1)
echo "negated: $?"
(exit 1) || echo "or ran"
(exit 0) && echo "and ran"
(exit abc) 2>&1
echo "bad exit: $?"
# A program's exit status is truncated to a byte too
sed Q888 <<<x
echo "sed Q888: $?"
echo x | sed Q257 | cat
echo "piped Q257: ${PIPESTATUS[*]}"
