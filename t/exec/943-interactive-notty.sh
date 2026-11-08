# An interactive shell reads input that is no terminal with the editor,
# as bash's readline does: what is typed is echoed on standard error
# after the prompt, the end of input says exit, and editing keys work
# (C-p, C-r searching, C-o running a line and fetching the next)
exec 2>&1
run() {
    printf "$1" | PS1='$ ' bash --norc -i 2>&1 | sed '/terminal process group/d; /no job control/d'
}
run 'echo one\necho two\n'
echo ---
printf 'echo one\necho two\n\020\020\n\022one\n' | bash --norc -i 2>/dev/null
echo ---
printf 'echo a\necho b\necho c\n\020\020\017\017\n' | bash --norc -i 2>/dev/null
