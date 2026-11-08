== stdout 1586
abort
accept-line
alias-expand-line
176

"\C-g": abort
"\C-x\C-g": abort
"\e\C-g": abort
"\C-j": accept-line
"\C-m": accept-line
# alias-expand-line (not bound)
# arrow-key-prefix (not bound)
# backward-byte (not bound)
"\C-b": backward-char
"\eOD": backward-char
"\e[D": backward-char
93

abort can be found on "\C-g", "\C-x\C-g", "\e\C-g".
accept-line can be found on "\C-j", "\C-m".
alias-expand-line is not bound to any keys
digit-argument can be found on "\e-", "\e0", "\e1", "\e2", "\e3", ...
insert-last-argument can be found on "\e.", "\e_".
self-insert can be found on " ", "!", "\"", "#", "$", ...
tab-insert is not bound to any keys
yank-last-arg can be found on "\e.", "\e_".
set bind-tty-special-chars on
set blink-matching-paren off
set byte-oriented off
set colored-completion-prefix off
set colored-stats off
keyseq-timeout is set to `500'
vi-cmd-mode-string is set to `(cmd)'
vi-ins-mode-string is set to `(ins)'
forward-char can be invoked via "\C-f", "\eOC", "\e[C".
q 0
tab-insert is not bound to any keys.
q 1
q 1
tab-insert can be invoked via "\C-xq".
"\C-xq": tab-insert
tab-insert is not bound to any keys.
"\C-xm": "hello world"
\C-xm outputs hello world
"\C-xx" "echo hi"
r 0
set completion-ignore-case on
bell-style is set to `none'
self-insert can be invoked via "\C-a", "\C-b", "\C-c", "\C-e", "\C-f", ...

abort can be found on "\C-g".
accept-line can be found on "\C-j", "\C-m".
m 1
z 2
bad 0
nonsense 0
forward-word can be invoked via "\C-xf", "\e[1;3C", "\e[1;5C", "\ef".
set mark-directories off
f 1

"\C-g": abort
# accept-line (not bound)
x 1
none 0

== stderr 2861
bash: line 4: bind: warning: line editing not enabled
bash: line 4: bind: warning: line editing not enabled
bash: line 5: bind: warning: line editing not enabled
bash: line 6: bind: warning: line editing not enabled
bash: line 7: bind: warning: line editing not enabled
bash: line 8: bind: warning: line editing not enabled
bash: line 9: bind: warning: line editing not enabled
bash: line 9: bind: warning: line editing not enabled
bash: line 10: bind: warning: line editing not enabled
bash: line 11: bind: warning: line editing not enabled
bash: line 12: bind: warning: line editing not enabled
bash: line 12: bind: `no-such-function': unknown function name
bash: line 13: bind: warning: line editing not enabled
bash: line 13: bind: warning: line editing not enabled
bash: line 14: bind: warning: line editing not enabled
bash: line 15: bind: warning: line editing not enabled
bash: line 15: bind: warning: line editing not enabled
bash: line 16: bind: warning: line editing not enabled
bash: line 16: bind: warning: line editing not enabled
bash: line 16: bind: warning: line editing not enabled
bash: line 17: bind: warning: line editing not enabled
bash: line 17: bind: warning: line editing not enabled
bash: line 17: bind: warning: line editing not enabled
bash: line 18: bind: warning: line editing not enabled
bash: line 18: bind: warning: line editing not enabled
bash: line 19: bind: warning: line editing not enabled
bash: line 19: bind: warning: line editing not enabled
bash: line 20: bind: warning: line editing not enabled
bash: line 20: bind: warning: line editing not enabled
bash: line 21: bind: warning: line editing not enabled
bash: line 21: bind: warning: line editing not enabled
bash: line 22: bind: warning: line editing not enabled
bash: line 23: bind: warning: line editing not enabled
bash: line 24: bind: warning: line editing not enabled
bash: line 24: bind: `nokeymap': invalid keymap name
bash: line 25: bind: warning: line editing not enabled
bash: line 25: bind: -z: invalid option
bind: usage: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
bash: line 26: bind: warning: line editing not enabled
bash: line 27: bind: warning: line editing not enabled
readline: nonsense: no key sequence terminator
bash: line 29: bind: warning: line editing not enabled
bash: line 29: bind: warning: line editing not enabled
bash: line 29: bind: warning: line editing not enabled
bash: line 30: bind: warning: line editing not enabled
bash: line 30: bind: nofile: cannot read: No such file or directory
bash: line 31: bind: warning: line editing not enabled
bash: line 32: bind: warning: line editing not enabled
bash: line 32: bind: nocolon: first non-whitespace character is not `"'
bash: line 33: bind: warning: line editing not enabled

== merged 4447
bash: line 4: bind: warning: line editing not enabled
abort
accept-line
alias-expand-line
bash: line 4: bind: warning: line editing not enabled
176
bash: line 5: bind: warning: line editing not enabled

"\C-g": abort
"\C-x\C-g": abort
"\e\C-g": abort
"\C-j": accept-line
"\C-m": accept-line
# alias-expand-line (not bound)
# arrow-key-prefix (not bound)
# backward-byte (not bound)
"\C-b": backward-char
"\eOD": backward-char
"\e[D": backward-char
bash: line 6: bind: warning: line editing not enabled
93
bash: line 7: bind: warning: line editing not enabled

abort can be found on "\C-g", "\C-x\C-g", "\e\C-g".
accept-line can be found on "\C-j", "\C-m".
alias-expand-line is not bound to any keys
bash: line 8: bind: warning: line editing not enabled
digit-argument can be found on "\e-", "\e0", "\e1", "\e2", "\e3", ...
insert-last-argument can be found on "\e.", "\e_".
self-insert can be found on " ", "!", "\"", "#", "$", ...
tab-insert is not bound to any keys
yank-last-arg can be found on "\e.", "\e_".
bash: line 9: bind: warning: line editing not enabled
set bind-tty-special-chars on
set blink-matching-paren off
set byte-oriented off
set colored-completion-prefix off
set colored-stats off
bash: line 9: bind: warning: line editing not enabled
keyseq-timeout is set to `500'
vi-cmd-mode-string is set to `(cmd)'
vi-ins-mode-string is set to `(ins)'
bash: line 10: bind: warning: line editing not enabled
forward-char can be invoked via "\C-f", "\eOC", "\e[C".
q 0
bash: line 11: bind: warning: line editing not enabled
tab-insert is not bound to any keys.
q 1
bash: line 12: bind: warning: line editing not enabled
bash: line 12: bind: `no-such-function': unknown function name
q 1
bash: line 13: bind: warning: line editing not enabled
bash: line 13: bind: warning: line editing not enabled
tab-insert can be invoked via "\C-xq".
bash: line 14: bind: warning: line editing not enabled
"\C-xq": tab-insert
bash: line 15: bind: warning: line editing not enabled
bash: line 15: bind: warning: line editing not enabled
tab-insert is not bound to any keys.
bash: line 16: bind: warning: line editing not enabled
bash: line 16: bind: warning: line editing not enabled
"\C-xm": "hello world"
bash: line 16: bind: warning: line editing not enabled
\C-xm outputs hello world
bash: line 17: bind: warning: line editing not enabled
bash: line 17: bind: warning: line editing not enabled
bash: line 17: bind: warning: line editing not enabled
"\C-xx" "echo hi"
bash: line 18: bind: warning: line editing not enabled
bash: line 18: bind: warning: line editing not enabled
r 0
bash: line 19: bind: warning: line editing not enabled
bash: line 19: bind: warning: line editing not enabled
bash: line 20: bind: warning: line editing not enabled
bash: line 20: bind: warning: line editing not enabled
set completion-ignore-case on
bash: line 21: bind: warning: line editing not enabled
bash: line 21: bind: warning: line editing not enabled
bell-style is set to `none'
bash: line 22: bind: warning: line editing not enabled
self-insert can be invoked via "\C-a", "\C-b", "\C-c", "\C-e", "\C-f", ...
bash: line 23: bind: warning: line editing not enabled

abort can be found on "\C-g".
accept-line can be found on "\C-j", "\C-m".
bash: line 24: bind: warning: line editing not enabled
bash: line 24: bind: `nokeymap': invalid keymap name
m 1
bash: line 25: bind: warning: line editing not enabled
bash: line 25: bind: -z: invalid option
bind: usage: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
z 2
bash: line 26: bind: warning: line editing not enabled
bad 0
bash: line 27: bind: warning: line editing not enabled
readline: nonsense: no key sequence terminator
nonsense 0
bash: line 29: bind: warning: line editing not enabled
bash: line 29: bind: warning: line editing not enabled
forward-word can be invoked via "\C-xf", "\e[1;3C", "\e[1;5C", "\ef".
bash: line 29: bind: warning: line editing not enabled
set mark-directories off
bash: line 30: bind: warning: line editing not enabled
bash: line 30: bind: nofile: cannot read: No such file or directory
f 1
bash: line 31: bind: warning: line editing not enabled

"\C-g": abort
# accept-line (not bound)
bash: line 32: bind: warning: line editing not enabled
bash: line 32: bind: nocolon: first non-whitespace character is not `"'
x 1
bash: line 33: bind: warning: line editing not enabled
none 0

== status 0
