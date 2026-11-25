# A function the environment gave that failed to import is passed on
# as it came, among the variables; one exported since comes after it,
# with the functions, as bash lists them: a child imports the second.
B=$(command -v bash)
H='BASH_FUNC_h%%=() { echo h; }; echo inj'
env -i "$H" A=1 PATH="$PATH" "$B" -c 'h() { echo new; }; export -f h; env | grep BASH_FUNC' 2>&1
echo --
env -i "$H" PATH="$PATH" "$B" -c 'h() { echo new; }; export -f h; bash -c h' 2>&1
echo --
env -i "$H" PATH="$PATH" "$B" -c 'h() { echo new; }; export -f h; unset -f h; env | grep BASH_FUNC' 2>&1
