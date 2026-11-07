== stdout 537
enable .
enable :
enable break
enable continue
enable eval
enable exec
enable exit
enable export
enable readonly
enable return
enable set
enable shift
enable source
enable times
enable trap
enable unset
enable .
enable :
enable break
status 0
enable -n echo
enable -n test
enable -n echo
enable -n test
echo is hashed (/opt/fakes/bin/echo)
status 0
hi
status 0
status 1
status 1
enable -n echo
enable -n test
sub
status 0
back
0
status 1
status 1
status 1
status 2
status 2
status 127
bash: line 22: builtin: enable: not a shell builtin

== stderr 516
bash: line 10: builtin: echo: not a shell builtin
bash: line 16: enable: nosuch: not a shell builtin
bash: line 16: enable: other: not a shell builtin
bash: line 17: enable: notbuiltin: not a shell builtin
bash: line 18: enable: test: not dynamically loaded
bash: line 19: enable: -x: invalid option
enable: usage: enable [-a] [-dnps] [-f filename] [name ...]
bash: line 20: enable: -f: option requires an argument
enable: usage: enable [-a] [-dnps] [-f filename] [name ...]
bash: line 21: enable: command not found

== merged 1053
enable .
enable :
enable break
enable continue
enable eval
enable exec
enable exit
enable export
enable readonly
enable return
enable set
enable shift
enable source
enable times
enable trap
enable unset
enable .
enable :
enable break
status 0
enable -n echo
enable -n test
enable -n echo
enable -n test
echo is hashed (/opt/fakes/bin/echo)
status 0
hi
status 0
bash: line 10: builtin: echo: not a shell builtin
status 1
status 1
enable -n echo
enable -n test
sub
status 0
back
0
bash: line 16: enable: nosuch: not a shell builtin
bash: line 16: enable: other: not a shell builtin
status 1
bash: line 17: enable: notbuiltin: not a shell builtin
status 1
bash: line 18: enable: test: not dynamically loaded
status 1
bash: line 19: enable: -x: invalid option
enable: usage: enable [-a] [-dnps] [-f filename] [name ...]
status 2
bash: line 20: enable: -f: option requires an argument
enable: usage: enable [-a] [-dnps] [-f filename] [name ...]
status 2
bash: line 21: enable: command not found
status 127
bash: line 22: builtin: enable: not a shell builtin

== status 0
