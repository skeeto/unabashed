# bash parses its options before it imports the environment's
# functions: -v echoes the text of each, and POSIX mode (--posix, -o
# posix, POSIXLY_CORRECT) refuses a name that is not an identifier.
# -n and -p import none, passing them on. SHELLOPTS comes later.
B=$(command -v bash)
F1='BASH_FUNC_f%%=() { echo f; }'
F2='BASH_FUNC_g%%=() {  echo g
}'
F3='BASH_FUNC_a-b%%=() { echo ab; }'
F4='BASH_FUNC_h%%=() { echo h; }; echo inj'
echo 1; env -i "$F1" "$F2" "$B" -v -c 'f; g' 2>&1
echo 2; env -i "$F1" "$F4" "$B" -o verbose -c 'f' 2>&1
echo 3; env -i "$F1" "$B" --verbose -c ':' 2>&1
echo 4; env -i "$F1" "$B" -xv +v -c ':' 2>&1
echo 5; env -i "$F1" "$B" -c -v ':' 2>&1
echo 6; env -i "$F1" SHELLOPTS=verbose "$B" -c 'f' 2>&1
echo 7; env -i "$F1" "$B" -v -o nonexist -c ':' 2>&1
echo 8; env -i "$F3" "$F1" "$B" --posix -c 'f; a-b; echo $?' 2>&1
echo 9; env -i "$F3" "$B" -o posix -c 'a-b; echo $?' 2>&1
echo 10; env -i "$F3" POSIXLY_CORRECT=1 "$B" -c 'a-b; echo $?' 2>&1
echo 11; env -i "$F3" SHELLOPTS=posix "$B" -c 'a-b; echo $?' 2>&1
echo 12; env -i "$F3" "$B" -c 'set -o posix; a-b; echo $?' 2>&1
echo 13; env -i "$F3" "$B" --posix -c 'printenv "BASH_FUNC_a-b%%"' 2>&1
echo 14; env -i "$F1" "$B" -p -c 'declare -F; printenv "BASH_FUNC_f%%"; bash -c f' 2>&1
echo 15; env -i "$F1" "$B" -nv -c 'f' 2>&1
