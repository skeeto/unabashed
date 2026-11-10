# personality: msys
# Hand-checked baseline: completion-ignore-case is off in msys mode, as
# in bash on MSYS2 (inputrc.md §7); native mode turns it on (047).
bind -v 2>/dev/null | grep completion-ignore-case
