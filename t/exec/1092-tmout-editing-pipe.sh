# TMOUT ends an interactive shell whose editor reads a pipe: the message
# follows the prompt directly, with no carriage return before it
: >.bash_logout
{ echo 'TMOUT=1'; sleep 2; } |
    HOME=. PS1='$ ' bash --noprofile --norc -il 2>&1 |
    sed '/job control/d; /terminal process group/d' | od -c
echo "status $?"
{ echo 'TMOUT=1'; echo 'echo abc'; sleep 2; } |
    HOME=. PS1='$ ' bash --noprofile --norc -il 2>/dev/null | od -c
