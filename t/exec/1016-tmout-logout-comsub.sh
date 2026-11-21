# TMOUT ends an interactive login shell waiting for a command, which
# runs ~/.bash_logout: a command substitution there loses none of the
# rest of the file
printf '%s\n' 'x=$(echo hello)' 'echo "x=$x src=${BASH_SOURCE[0]}"' \
    'echo logout-end' >.bash_logout
{ echo 'TMOUT=1'; sleep 2; } |
    HOME=. PS1='$ ' bash --noprofile --norc --noediting -il 2>&1 |
    sed '/job control/d; /terminal process group/d'
echo "status $?"
