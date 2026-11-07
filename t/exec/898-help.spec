== stdout 8846
 ! PIPELINE                              history [-c] [-d offset] [n] or hist>
 job_spec [&]                            if COMMANDS; then COMMANDS; [ elif C>
 (( expression ))                        jobs [-lnprs] [jobspec ...] or jobs >
 . [-p path] filename [arguments]        kill [-s sigspec | -n signum | -sigs>
 :                                       let arg [arg ...]
 [ arg... ]                              local [option] name[=value] ...
 [[ expression ]]                        logout [n]
 alias [-p] [name[=value] ... ]          mapfile [-d delim] [-n count] [-O or>
 bg [job_spec ...]                       popd [-n] [+N | -N]
 bind [-lpsvPSVX] [-m keymap] [-f file>  printf [-v var] format [arguments]
 break [n]                               pushd [-n] [+N | -N | dir]
 builtin [shell-builtin [arg ...]]       pwd [-LP]
 caller [expr]                           read [-Eers] [-a array] [-d delim] [>
 case WORD in [PATTERN [| PATTERN]...)>  readarray [-d delim] [-n count] [-O >
 cd [-L|[-P [-e]]] [-@] [dir]            readonly [-aAf] [name[=value] ...] o>
 command [-pVv] command [arg ...]        return [n]
 compgen [-V varname] [-abcdefgjksuv] >  select NAME [in WORDS ... ;] do COMM>
 complete [-abcdefgjksuv] [-pr] [-DEI]>  set [-abefhkmnptuvxBCEHPT] [-o optio>
 compopt [-o|+o option] [-DEI] [name .>  shift [n]
 continue [n]                            shopt [-pqsu] [-o] [optname ...]
 coproc [NAME] command [redirections]    source [-p path] filename [arguments>
 declare [-aAfFgiIlnrtux] [name[=value>  suspend [-f]
 dirs [-clpv] [+N] [-N]                  test [expr]
 disown [-h] [-ar] [jobspec ... | pid >  time [-p] pipeline
 echo [-neE] [arg ...]                   times
 enable [-a] [-dnps] [-f filename] [na>  trap [-Plp] [[action] signal_spec ..>
 eval [arg ...]                          true
 exec [-cl] [-a name] [command [argume>  type [-afptP] name [name ...]
 exit [n]                                typeset [-aAfFgiIlnrtux] name[=value>
 export [-fn] [name[=value] ...] or ex>  ulimit [-SHabcdefiklmnpqrstuvxPRT] [>
 false                                   umask [-p] [-S] [mode]
 fc [-e ename] [-lnr] [first] [last] o>  unalias [-a] name [name ...]
 fg [job_spec]                           unset [-f] [-v] [-n] [name ...]
 for NAME [in WORDS ... ] ; do COMMAND>  until COMMANDS; do COMMANDS-2; done
 for (( exp1; exp2; exp3 )); do COMMAN>  variables - Names and meanings of so>
 function name { COMMANDS ; } or name >  wait [-fn] [-p var] [id ...]
 getopts optstring name [arg ...]        while COMMANDS; do COMMANDS-2; done
 hash [-lr] [-p pathname] [-dt] [name >  { COMMANDS ; }
 help [-dms] [pattern ...]
 ! PIPELINE                    history [-c] [-d offset] [>
 job_spec [&]                  if COMMANDS; then COMMANDS>
 (( expression ))              jobs [-lnprs] [jobspec ...>
 . [-p path] filename [argum>  kill [-s sigspec | -n sign>
 :                             let arg [arg ...]
 ! PIPELINE                                                                                          history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
 job_spec [&]                                                                                        if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi
 (( expression ))                                                                                    jobs [-lnprs] [jobspec ...] or jobs -x command [args]
Shell commands matching keyword `*'

!: ! PIPELINE
%: job_spec [&]
(( ... )): (( expression ))
.: . [-p path] filename [arguments]
:: :
[: [ arg... ]
[[ ... ]]: [[ expression ]]
alias: alias [-p] [name[=value] ... ]
bg: bg [job_spec ...]
bind: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
break: break [n]
builtin: builtin [shell-builtin [arg ...]]
caller: caller [expr]
case: case WORD in [PATTERN [| PATTERN]...) COMMANDS ;;]... esac
cd: cd [-L|[-P [-e]]] [-@] [dir]
command: command [-pVv] command [arg ...]
compgen: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
complete: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
compopt: compopt [-o|+o option] [-DEI] [name ...]
continue: continue [n]
coproc: coproc [NAME] command [redirections]
declare: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
dirs: dirs [-clpv] [+N] [-N]
disown: disown [-h] [-ar] [jobspec ... | pid ...]
echo: echo [-neE] [arg ...]
enable: enable [-a] [-dnps] [-f filename] [name ...]
eval: eval [arg ...]
exec: exec [-cl] [-a name] [command [argument ...]] [redirection ...]
exit: exit [n]
export: export [-fn] [name[=value] ...] or export -p [-f]
false: false
fc: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
fg: fg [job_spec]
for: for NAME [in WORDS ... ] ; do COMMANDS; done
for ((: for (( exp1; exp2; exp3 )); do COMMANDS; done
function: function name { COMMANDS ; } or name () { COMMANDS ; }
getopts: getopts optstring name [arg ...]
hash: hash [-lr] [-p pathname] [-dt] [name ...]
help: help [-dms] [pattern ...]
history: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
if: if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi
jobs: jobs [-lnprs] [jobspec ...] or jobs -x command [args]
kill: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]
let: let arg [arg ...]
local: local [option] name[=value] ...
logout: logout [n]
mapfile: mapfile [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
popd: popd [-n] [+N | -N]
printf: printf [-v var] format [arguments]
pushd: pushd [-n] [+N | -N | dir]
pwd: pwd [-LP]
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
return: return [n]
select: select NAME [in WORDS ... ;] do COMMANDS; done
set: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]
shift: shift [n]
shopt: shopt [-pqsu] [-o] [optname ...]
source: source [-p path] filename [arguments]
suspend: suspend [-f]
test: test [expr]
time: time [-p] pipeline
times: times
trap: trap [-Plp] [[action] signal_spec ...]
true: true
type: type [-afptP] name [name ...]
typeset: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
ulimit: ulimit [-SHabcdefiklmnpqrstuvxPRT] [limit]
umask: umask [-p] [-S] [mode]
unalias: unalias [-a] name [name ...]
unset: unset [-f] [-v] [-n] [name ...]
until: until COMMANDS; do COMMANDS-2; done
variables: variables - Names and meanings of some shell variables
wait: wait [-fn] [-p var] [id ...]
while: while COMMANDS; do COMMANDS-2; done
{ ... }: { COMMANDS ; }
status 0
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
for: for NAME [in WORDS ... ] ; do COMMANDS; done
[: [ arg... ]
[[ ... ]]: [[ expression ]]
{ ... }: { COMMANDS ; }
Shell commands matching keywords `r*, z*'

read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
return: return [n]
status 0
Shell commands

read -
readarray -
readonly -
status 1
status 2
status 1
 ! PIPELINE                              history [-c] [-d offset] [n] or hist>
.: . [-p path] filename [arguments]
. 2
alias: alias [-p] [name[=value] ... ]
alias 2
cd: cd [-L|[-P [-e]]] [-@] [dir]
cd 2
shift: shift [n]
shift 2
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
read 2
wait: wait [-fn] [-p var] [id ...]
wait 2
source: source [-p path] filename [arguments]
source 2
typeset: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
typeset 2
help: help [-dms] [pattern ...]
help 2
: 0
--help
echo 0
true 0
false 1
test 0
[ 2
shift: shift [n]
help: help [-dms] [pattern ...]
*echo [-neE] [arg ...]                   times
status 1
NAME
SYNOPSIS
    true
DESCRIPTION
SEE ALSO
    bash(1)

IMPLEMENTATION
true -
false -
status 0

== stderr 351
bash: line 11: help: no help topics match `zz'.  Try `help help' or `man -k zz' or `info zz'.
bash: line 12: help: -x: invalid option
help: usage: help [-dms] [pattern ...]
bash: line 13: help: no help topics match `-x'.  Try `help help' or `man -k -x' or `info -x'.
bash: line 19: [: missing `]'
bash: line 22: help: write error: Bad file descriptor

== merged 9197
 ! PIPELINE                              history [-c] [-d offset] [n] or hist>
 job_spec [&]                            if COMMANDS; then COMMANDS; [ elif C>
 (( expression ))                        jobs [-lnprs] [jobspec ...] or jobs >
 . [-p path] filename [arguments]        kill [-s sigspec | -n signum | -sigs>
 :                                       let arg [arg ...]
 [ arg... ]                              local [option] name[=value] ...
 [[ expression ]]                        logout [n]
 alias [-p] [name[=value] ... ]          mapfile [-d delim] [-n count] [-O or>
 bg [job_spec ...]                       popd [-n] [+N | -N]
 bind [-lpsvPSVX] [-m keymap] [-f file>  printf [-v var] format [arguments]
 break [n]                               pushd [-n] [+N | -N | dir]
 builtin [shell-builtin [arg ...]]       pwd [-LP]
 caller [expr]                           read [-Eers] [-a array] [-d delim] [>
 case WORD in [PATTERN [| PATTERN]...)>  readarray [-d delim] [-n count] [-O >
 cd [-L|[-P [-e]]] [-@] [dir]            readonly [-aAf] [name[=value] ...] o>
 command [-pVv] command [arg ...]        return [n]
 compgen [-V varname] [-abcdefgjksuv] >  select NAME [in WORDS ... ;] do COMM>
 complete [-abcdefgjksuv] [-pr] [-DEI]>  set [-abefhkmnptuvxBCEHPT] [-o optio>
 compopt [-o|+o option] [-DEI] [name .>  shift [n]
 continue [n]                            shopt [-pqsu] [-o] [optname ...]
 coproc [NAME] command [redirections]    source [-p path] filename [arguments>
 declare [-aAfFgiIlnrtux] [name[=value>  suspend [-f]
 dirs [-clpv] [+N] [-N]                  test [expr]
 disown [-h] [-ar] [jobspec ... | pid >  time [-p] pipeline
 echo [-neE] [arg ...]                   times
 enable [-a] [-dnps] [-f filename] [na>  trap [-Plp] [[action] signal_spec ..>
 eval [arg ...]                          true
 exec [-cl] [-a name] [command [argume>  type [-afptP] name [name ...]
 exit [n]                                typeset [-aAfFgiIlnrtux] name[=value>
 export [-fn] [name[=value] ...] or ex>  ulimit [-SHabcdefiklmnpqrstuvxPRT] [>
 false                                   umask [-p] [-S] [mode]
 fc [-e ename] [-lnr] [first] [last] o>  unalias [-a] name [name ...]
 fg [job_spec]                           unset [-f] [-v] [-n] [name ...]
 for NAME [in WORDS ... ] ; do COMMAND>  until COMMANDS; do COMMANDS-2; done
 for (( exp1; exp2; exp3 )); do COMMAN>  variables - Names and meanings of so>
 function name { COMMANDS ; } or name >  wait [-fn] [-p var] [id ...]
 getopts optstring name [arg ...]        while COMMANDS; do COMMANDS-2; done
 hash [-lr] [-p pathname] [-dt] [name >  { COMMANDS ; }
 help [-dms] [pattern ...]
 ! PIPELINE                    history [-c] [-d offset] [>
 job_spec [&]                  if COMMANDS; then COMMANDS>
 (( expression ))              jobs [-lnprs] [jobspec ...>
 . [-p path] filename [argum>  kill [-s sigspec | -n sign>
 :                             let arg [arg ...]
 ! PIPELINE                                                                                          history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
 job_spec [&]                                                                                        if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi
 (( expression ))                                                                                    jobs [-lnprs] [jobspec ...] or jobs -x command [args]
Shell commands matching keyword `*'

!: ! PIPELINE
%: job_spec [&]
(( ... )): (( expression ))
.: . [-p path] filename [arguments]
:: :
[: [ arg... ]
[[ ... ]]: [[ expression ]]
alias: alias [-p] [name[=value] ... ]
bg: bg [job_spec ...]
bind: bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]
break: break [n]
builtin: builtin [shell-builtin [arg ...]]
caller: caller [expr]
case: case WORD in [PATTERN [| PATTERN]...) COMMANDS ;;]... esac
cd: cd [-L|[-P [-e]]] [-@] [dir]
command: command [-pVv] command [arg ...]
compgen: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
complete: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
compopt: compopt [-o|+o option] [-DEI] [name ...]
continue: continue [n]
coproc: coproc [NAME] command [redirections]
declare: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
dirs: dirs [-clpv] [+N] [-N]
disown: disown [-h] [-ar] [jobspec ... | pid ...]
echo: echo [-neE] [arg ...]
enable: enable [-a] [-dnps] [-f filename] [name ...]
eval: eval [arg ...]
exec: exec [-cl] [-a name] [command [argument ...]] [redirection ...]
exit: exit [n]
export: export [-fn] [name[=value] ...] or export -p [-f]
false: false
fc: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
fg: fg [job_spec]
for: for NAME [in WORDS ... ] ; do COMMANDS; done
for ((: for (( exp1; exp2; exp3 )); do COMMANDS; done
function: function name { COMMANDS ; } or name () { COMMANDS ; }
getopts: getopts optstring name [arg ...]
hash: hash [-lr] [-p pathname] [-dt] [name ...]
help: help [-dms] [pattern ...]
history: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
if: if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi
jobs: jobs [-lnprs] [jobspec ...] or jobs -x command [args]
kill: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]
let: let arg [arg ...]
local: local [option] name[=value] ...
logout: logout [n]
mapfile: mapfile [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
popd: popd [-n] [+N | -N]
printf: printf [-v var] format [arguments]
pushd: pushd [-n] [+N | -N | dir]
pwd: pwd [-LP]
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
return: return [n]
select: select NAME [in WORDS ... ;] do COMMANDS; done
set: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]
shift: shift [n]
shopt: shopt [-pqsu] [-o] [optname ...]
source: source [-p path] filename [arguments]
suspend: suspend [-f]
test: test [expr]
time: time [-p] pipeline
times: times
trap: trap [-Plp] [[action] signal_spec ...]
true: true
type: type [-afptP] name [name ...]
typeset: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
ulimit: ulimit [-SHabcdefiklmnpqrstuvxPRT] [limit]
umask: umask [-p] [-S] [mode]
unalias: unalias [-a] name [name ...]
unset: unset [-f] [-v] [-n] [name ...]
until: until COMMANDS; do COMMANDS-2; done
variables: variables - Names and meanings of some shell variables
wait: wait [-fn] [-p var] [id ...]
while: while COMMANDS; do COMMANDS-2; done
{ ... }: { COMMANDS ; }
status 0
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
for: for NAME [in WORDS ... ] ; do COMMANDS; done
[: [ arg... ]
[[ ... ]]: [[ expression ]]
{ ... }: { COMMANDS ; }
Shell commands matching keywords `r*, z*'

read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
readarray: readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]
readonly: readonly [-aAf] [name[=value] ...] or readonly -p
return: return [n]
status 0
Shell commands

read -
readarray -
readonly -
bash: line 11: help: no help topics match `zz'.  Try `help help' or `man -k zz' or `info zz'.
status 1
bash: line 12: help: -x: invalid option
help: usage: help [-dms] [pattern ...]
status 2
bash: line 13: help: no help topics match `-x'.  Try `help help' or `man -k -x' or `info -x'.
status 1
 ! PIPELINE                              history [-c] [-d offset] [n] or hist>
.: . [-p path] filename [arguments]
. 2
alias: alias [-p] [name[=value] ... ]
alias 2
cd: cd [-L|[-P [-e]]] [-@] [dir]
cd 2
shift: shift [n]
shift 2
read: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
read 2
wait: wait [-fn] [-p var] [id ...]
wait 2
source: source [-p path] filename [arguments]
source 2
typeset: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
typeset 2
help: help [-dms] [pattern ...]
help 2
: 0
--help
echo 0
true 0
false 1
test 0
bash: line 19: [: missing `]'
[ 2
shift: shift [n]
help: help [-dms] [pattern ...]
*echo [-neE] [arg ...]                   times
bash: line 22: help: write error: Bad file descriptor
status 1
NAME
SYNOPSIS
    true
DESCRIPTION
SEE ALSO
    bash(1)

IMPLEMENTATION
true -
false -
status 0

== status 0
