== stdout 1960
complete -o filenames -o nospace -F _f foo
complete -W 'i' -I
complete -o filenames -o nospace -F _f 'b ar'
complete -W 'd' -D
complete -W 'e' -E
complete -o filenames -o nospace -F _f ''
st 0
complete -W '1' e
complete -W '1' d
complete -W '1' c
complete -W '1' b
complete -W '1' a
complete -W '1' e
complete -W '1' d
complete -W '2' c
complete -W '3' b
complete -W '1' a
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
complete -o bashdefault -o default -o dirnames -o fullquote -o noquote -o nosort -o plusdirs -b -c -d -e -f -j -k -s -u -A arrayvar -A binding -A disabled -A enabled -A function -A helptopic -A running -A setopt -A shopt -A stopped x
complete -W 'a' 'x$y'
complete -W 'a' '~a'
complete -W 'a' a~
complete -W 'a' '#a'
complete -W 'a' a#
complete -W 'a' \'
complete -W 'a' ''
complete -F 'f$x' y
complete -W \' q
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
st 1
st 1
st 1
st 1
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
st 2
st 2
st 2
st 2
st 2
st 2
st 2
st 0
st 1
complete -o filenames -o nospace -W 'x' sb
complete -W 'y' sa
complete -o nospace -W 'x' sb
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs sa
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort -o nospace +o plusdirs sb
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote -o nosort +o nospace +o plusdirs sa
st 1
st 1
st 1
st 1
st 2
st 2
st 2
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs -D
compopt +o bashdefault -o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs -D
3

== stderr 1928
bash: line 18: complete: -E: no completion specification
bash: line 19: complete: nope: no completion specification
bash: line 20: complete: nope: no completion specification
bash: line 21: complete: _InitialWorD_: no completion specification
bash: line 22: complete: _DefaultCmD_: no completion specification
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
bash: line 25: complete: bogus: invalid action name
bash: line 26: complete: bogus: invalid option name
bash: line 27: complete: -z: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
bash: line 28: complete: -A: option requires an argument
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
bash: line 29: complete: `a b': not a valid identifier
bash: line 30: complete: -V: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
bash: line 32: complete: foo: no completion specification
bash: line 40: compopt: nope: no completion specification
bash: line 41: compopt: _DefaultCmD_: no completion specification
bash: line 42: compopt: not currently executing completion function
bash: line 43: compopt: not currently executing completion function
bash: line 44: compopt: -p: invalid option
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
bash: line 45: compopt: bogus: invalid option name
bash: line 46: compopt: -o: option requires an argument
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]

== merged 3888
complete -o filenames -o nospace -F _f foo
complete -W 'i' -I
complete -o filenames -o nospace -F _f 'b ar'
complete -W 'd' -D
complete -W 'e' -E
complete -o filenames -o nospace -F _f ''
st 0
complete -W '1' e
complete -W '1' d
complete -W '1' c
complete -W '1' b
complete -W '1' a
complete -W '1' e
complete -W '1' d
complete -W '2' c
complete -W '3' b
complete -W '1' a
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
bash: line 18: complete: -E: no completion specification
complete -o bashdefault -o default -o dirnames -o fullquote -o noquote -o nosort -o plusdirs -b -c -d -e -f -j -k -s -u -A arrayvar -A binding -A disabled -A enabled -A function -A helptopic -A running -A setopt -A shopt -A stopped x
complete -W 'a' 'x$y'
complete -W 'a' '~a'
complete -W 'a' a~
complete -W 'a' '#a'
complete -W 'a' a#
complete -W 'a' \'
complete -W 'a' ''
complete -F 'f$x' y
complete -W \' q
bash: line 19: complete: nope: no completion specification
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
st 1
bash: line 20: complete: nope: no completion specification
st 1
bash: line 21: complete: _InitialWorD_: no completion specification
st 1
bash: line 22: complete: _DefaultCmD_: no completion specification
st 1
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
st 2
bash: line 25: complete: bogus: invalid action name
st 2
bash: line 26: complete: bogus: invalid option name
st 2
bash: line 27: complete: -z: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
st 2
bash: line 28: complete: -A: option requires an argument
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
st 2
bash: line 29: complete: `a b': not a valid identifier
st 2
bash: line 30: complete: -V: invalid option
complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
st 2
st 0
bash: line 32: complete: foo: no completion specification
st 1
complete -o filenames -o nospace -W 'x' sb
complete -W 'y' sa
complete -o nospace -W 'x' sb
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs sa
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort -o nospace +o plusdirs sb
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote -o nosort +o nospace +o plusdirs sa
bash: line 40: compopt: nope: no completion specification
st 1
bash: line 41: compopt: _DefaultCmD_: no completion specification
st 1
bash: line 42: compopt: not currently executing completion function
st 1
bash: line 43: compopt: not currently executing completion function
st 1
bash: line 44: compopt: -p: invalid option
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
st 2
bash: line 45: compopt: bogus: invalid option name
st 2
bash: line 46: compopt: -o: option requires an argument
compopt: usage: compopt [-o|+o option] [-DEI] [name ...]
st 2
compopt +o bashdefault +o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs -D
compopt +o bashdefault -o default +o dirnames +o filenames +o fullquote +o noquote +o nosort +o nospace +o plusdirs -D
3

== status 0
