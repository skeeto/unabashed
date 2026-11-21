== stdout 225
declare -a a=([9223372036854775807]="x")
declare assoc 0
declare -A h=([k]="w" )
complete -p 1
compopt -Z 2
compopt -o 2
compopt bogus 2
compopt nosuch 1
compgen -V 1
complete -Z 2
bind -Z 2
bind -m 1
bind -q 1
bind -u 1
end

== stderr 1455
bash: line 6: ro: readonly variable
bash: line 7: ro: readonly variable
bash: line 8: a[-9223372036854775808]: bad array subscript
bash: line 10: '': bad array subscript
bash: line 11: [@]=x: cannot assign to non-numeric index
bash: line 12: []=x: bad array subscript
bash: line 13: complete: nosuch: no completion specification
bash: line 14: compopt: -Z: invalid option
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
bash: line 15: compopt: -o: option requires an argument
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
bash: line 16: compopt: bogus: invalid option name
bash: line 17: compopt: nosuch: no completion specification
bash: line 19: complete: -Z: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
bash: line 20: bind: warning: line editing not enabled
bash: line 20: bind: -Z: invalid option
bind: usage: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
bash: line 21: bind: warning: line editing not enabled
bash: line 21: bind: `bogus': invalid keymap name
bash: line 22: bind: warning: line editing not enabled
bash: line 22: bind: `bogus': unknown function name
bash: line 23: bind: warning: line editing not enabled
bash: line 23: bind: `bogus': unknown function name

== merged 1680
bash: line 6: ro: readonly variable
bash: line 7: ro: readonly variable
bash: line 8: a[-9223372036854775808]: bad array subscript
declare -a a=([9223372036854775807]="x")
bash: line 10: '': bad array subscript
declare assoc 0
declare -A h=([k]="w" )
bash: line 11: [@]=x: cannot assign to non-numeric index
bash: line 12: []=x: bad array subscript
bash: line 13: complete: nosuch: no completion specification
complete -p 1
bash: line 14: compopt: -Z: invalid option
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
compopt -Z 2
bash: line 15: compopt: -o: option requires an argument
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
compopt -o 2
bash: line 16: compopt: bogus: invalid option name
compopt bogus 2
bash: line 17: compopt: nosuch: no completion specification
compopt nosuch 1
compgen -V 1
bash: line 19: complete: -Z: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
complete -Z 2
bash: line 20: bind: warning: line editing not enabled
bash: line 20: bind: -Z: invalid option
bind: usage: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
bind -Z 2
bash: line 21: bind: warning: line editing not enabled
bash: line 21: bind: `bogus': invalid keymap name
bind -m 1
bash: line 22: bind: warning: line editing not enabled
bash: line 22: bind: `bogus': unknown function name
bind -q 1
bash: line 23: bind: warning: line editing not enabled
bash: line 23: bind: `bogus': unknown function name
bind -u 1
end

== status 0
