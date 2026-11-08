# bind in a shell without line editing: it warns, and still reads and
# changes readline's keymaps, variables, macros and shell commands
INPUTRC=/dev/null
bind -l | head -3; bind -l | wc -l
bind -p | head -12
bind -p | grep -c 'not bound'
bind -P | head -4
bind -P | grep -E '^(self-insert|digit-argument|yank-last-arg|insert-last-argument|tab-insert) '
bind -v | head -5; bind -V | tail -3
bind -q forward-char; echo "q $?"
bind -q tab-insert; echo "q $?"
bind -q no-such-function; echo "q $?"
bind '"\C-xq": tab-insert'; bind -q tab-insert
bind -p | grep -F '"\C-xq"'
bind -u tab-insert; bind -q tab-insert
bind '"\C-xm":"hello world"'; bind -s; bind -S
bind -X; bind -x '"\C-xx": echo hi'; bind -X
bind -r '\C-xm'; bind -s; echo "r $?"
bind -r '\C-xx'; bind -X
bind 'set completion-ignore-case on'; bind -v | grep completion-ignore-case
bind 'set bell-style none'; bind -V | grep bell-style
bind -m vi-insert -q self-insert
bind -m vi-command -P | head -3
bind -m nokeymap -p; echo "m $?"
bind -z; echo "z $?"
bind '"\C-xa": no-such-function'; echo "bad $?"
bind 'nonsense'; echo "nonsense $?"
printf '"\\C-xf": forward-word\nset mark-directories off\n' >rc
bind -f rc; bind -q forward-word; bind -v | grep mark-directories
bind -f nofile; echo "f $?"
bind -p -m emacs-ctlx | head -3
bind -x 'nocolon'; echo "x $?"
bind; echo "none $?"
