# A failglob error abandons the rest of the current command line,
# including commands after ; and &&, then the script goes on.
cd /tmp
shopt -s failglob
echo a; echo *.none; echo b
echo "next $?"
true && echo *.none || echo c
echo "next $?"
{ echo d; echo *.none; echo e; }
echo "next $?"
f() { echo in-f; echo *.none; echo after-in-f; }
f; echo after-f
echo "next $?"
(echo sub; echo *.none; echo sub2); echo "sub status $?"
x=$(echo *.none; echo cs); echo "cs [$x] $?"
eval 'echo *.none; echo ev'; echo "eval $?"
if echo *.none; then echo then; fi; echo "if $?"
echo end
