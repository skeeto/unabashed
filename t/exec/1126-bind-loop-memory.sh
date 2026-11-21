# Binding the same keys and settings again and again takes no more
# memory: a loop as long as a prompt command's over a long session, in
# the editor's arena of a few MiB here
INPUTRC=/dev/null
exec 2>/dev/null
for ((i = 0; i < 100000; i++)); do
    bind '"\C-t": "a"'
    bind '"\C-t": "abcdefghijklmnopqrstuvwxyz'$i'"'
    bind -x '"\C-xr": echo '$i
    bind 'set comment-begin #'$i
    bind 'set vi-ins-mode-string "ins'$i'"'
done
bind -s; bind -X; bind -v | grep -E 'comment-begin|vi-ins-mode-string'
echo done
