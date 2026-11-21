# Input that is no terminal echoed by an interactive shell's editor on
# a terminal it takes as dumb scrolls sideways once the line passes the
# width, as readline draws it: the line drawn again from the left, with
# spaces over what is left of the old and backspaces back to the cursor
# (or nothing when it is there), not drawn a second time up to it. A
# last line with no newline is run as if it had one, which is echoed.
exec 2>&1
run() {
    printf "$2" | COLUMNS=20 PS1=$1 bash --norc -i 2>&1 |
        sed '/terminal process group/d; /no job control/d' | cat -v
}
run '$ ' 'echo abcdefghijklmnopqrstuvwxyz\necho 2\n'
echo ---
run 'abcdefghijklm$ ' 'echo one\n'
echo ---
run 'abcdefghijklmno$ ' 'echo one\necho two'
echo ---
run '$ ' 'echo three'
