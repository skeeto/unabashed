# ( ) subshell isolates function definitions and unset -f
f() { echo outer-f; }
(f() { echo inner-f; }; f)
f
(unset -f f; f 2>&1; echo "st=$?")
f
(g() { echo g; }; g)
g 2>&1
echo "g status: $?"
declare -F g
echo "declare -F g: $?"
