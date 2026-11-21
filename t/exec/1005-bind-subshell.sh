# A subshell's bindings and settings start as its parent's, and what it
# changes stays its own, as in bash's forked child: a command
# substitution, ( ), a pipeline stage, an asynchronous job, nested
INPUTRC=/dev/null
exec 2>/dev/null
bind 'set bell-style none'; bind '"\C-t": "hello"'
echo "sub: $(bind -v | grep bell-style) $(bind -s)"
x=$(bind 'set bell-style visible'; bind '"\C-t": "bye"'; bind -v | grep bell-style; bind -s)
echo "changed: $x"
bind -v | grep bell-style; bind -s
(bind 'set bell-style audible'; bind -r '\C-t'; bind -v | grep bell-style; bind -s; echo "( ) $?")
bind -v | grep bell-style; bind -s
{ bind '"\C-t": "piped"'; bind -s; } | cat
bind -s | cat; bind -s
bind '"\C-t": "async"' &
wait; bind -s
echo "$(bind '"\C-t": "outer"'; echo "$(bind '"\C-t": "inner"'; bind -s)"; bind -s)"
bind -s
set -o vi; echo "vi: $(set -o emacs; bind -v | grep editing-mode)"
bind -v | grep editing-mode
x=$(bind 'set editing-mode emacs'; set -o | grep -E '^(vi|emacs) ')
echo "$x"; set -o | grep -E '^(vi|emacs) '
