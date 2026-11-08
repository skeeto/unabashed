== stdout 1189
st 0
st 0
st 1
st 1
st 2
st 2
st 2
st 2
st 2
aa
ab
c
zz
zfun
x.c
zw
fee
fi
fum
if then else elif fi case esac for select while until do done in function time { } ! [[ ]] coproc 
allexport braceexpand emacs errexit errtrace functrace hashall histexpand history ignoreeof interactive-comments keyword monitor noclobber noexec noglob nolog notify nounset onecmd physical pipefail posix privileged verbose vi xtrace 
array_expand_once
assoc_expand_once
autocd
. : [ alias bg bind break builtin caller cd command compgen complete compopt continue declare dirs disown echo enable eval exec exit export false fc fg getopts hash help history jobs kill let local logout mapfile popd printf pushd pwd read readarray readonly return set shift shopt source suspend test times trap true type typeset ulimit umask unalias unset wait 
echo
echo
! % (( ... )) . : [ [[ ... ]] alias 
EXIT SIGHUP SIGINT SIGQUIT SIGILL SIGRTMAX-1 SIGRTMAX DEBUG ERR RETURN 
st 1
arr
as
zexp
abort
accept-line
arrow-key-prefix
c d
no_empty_cmd_completion
nocaseglob
val
a b
1
2
c
d
*.c
a
b c
a b
3$
a b$
foo
bar
baz
foo
fob
b
st 1
ab
st 1
bar
<b>
d1
d2
f1
f2
x.c
st 1
f1
f2
d1
d2
d1
d1
d2
d2
st 1
d1
d2
pdx
d1
d2
f1
f2
$HOME

== stderr 622
bash: line 7: compgen: -D: invalid option
compgen: usage: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
bash: line 8: compgen: -p: invalid option
compgen: usage: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
bash: line 9: compgen: `1x': not a valid identifier
bash: line 10: compgen: noaction: invalid action name
bash: line 11: compgen: nooption: invalid option name
bash: line 58: zz: boom

== merged 1811
st 0
st 0
st 1
st 1
bash: line 7: compgen: -D: invalid option
compgen: usage: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
st 2
bash: line 8: compgen: -p: invalid option
compgen: usage: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]
st 2
bash: line 9: compgen: `1x': not a valid identifier
st 2
bash: line 10: compgen: noaction: invalid action name
st 2
bash: line 11: compgen: nooption: invalid option name
st 2
aa
ab
c
zz
zfun
x.c
zw
fee
fi
fum
if then else elif fi case esac for select while until do done in function time { } ! [[ ]] coproc 
allexport braceexpand emacs errexit errtrace functrace hashall histexpand history ignoreeof interactive-comments keyword monitor noclobber noexec noglob nolog notify nounset onecmd physical pipefail posix privileged verbose vi xtrace 
array_expand_once
assoc_expand_once
autocd
. : [ alias bg bind break builtin caller cd command compgen complete compopt continue declare dirs disown echo enable eval exec exit export false fc fg getopts hash help history jobs kill let local logout mapfile popd printf pushd pwd read readarray readonly return set shift shopt source suspend test times trap true type typeset ulimit umask unalias unset wait 
echo
echo
! % (( ... )) . : [ [[ ... ]] alias 
EXIT SIGHUP SIGINT SIGQUIT SIGILL SIGRTMAX-1 SIGRTMAX DEBUG ERR RETURN 
st 1
arr
as
zexp
abort
accept-line
arrow-key-prefix
c d
no_empty_cmd_completion
nocaseglob
val
a b
1
2
c
d
*.c
a
b c
a b
3$
a b$
foo
bar
baz
foo
fob
b
st 1
ab
st 1
bar
<b>
d1
d2
f1
f2
x.c
st 1
f1
f2
d1
d2
d1
d1
d2
d2
st 1
d1
d2
pdx
d1
d2
f1
f2
$HOME
bash: line 58: zz: boom

== status 127
