# personality: native
# Hand-checked baseline: completion-ignore-case defaults to on in native
# mode, as in readline's native Windows builds (inputrc.md §7), and an
# init file can still turn it off. msys mode keeps bash's off (048).
bind -v 2>/dev/null | grep completion-ignore-case
bind 'set completion-ignore-case off'
bind -v 2>/dev/null | grep completion-ignore-case
