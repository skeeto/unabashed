# shell: --rcfile test.sh -i
# conrun: sleep:800 'type:\u0014' sleep:300 'type:ab\nc\u000a' sleep:300 'type:\u0018a' await:ready ctrl-c sleep:500 'type:echo st=$?\n' sleep:300 'type:\u0018b' await:ready2 ctrl-c sleep:500 'type:echo st=$?\n' sleep:300 'type:exit\n'
# bind -x runs its command with the console as readline leaves a
# terminal: neither line input nor echo, Enter a CR, but Ctrl-C still a
# signal to the console's processes, which abandons the command, even
# in the shell's own read of the console. The editor draws on the
# console (descriptor 2); the commands write to descriptor 1.
exec 2>CONOUT\$
PS1='$ '
bind -x '"\C-t": read x; printf "<%q>\n" "$x"'
bind -x '"\C-xa": : >ready; read x; echo "after $?"'
kid() { "$U" -c ': >ready2; while :; do :; done'; echo "child $?"; }
bind -x '"\C-xb": kid'
