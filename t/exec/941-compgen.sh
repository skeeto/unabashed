# compgen: actions, globs, word lists, filters, prefixes, directories
# and its statuses
compgen; echo "st $?"
compgen -- foo; echo "st $?"
compgen -W 'x y z' q; echo "st $?"
compgen -W ''; echo "st $?"
compgen -D; echo "st $?"
compgen -p; echo "st $?"
compgen -V 1x -b; echo "st $?"
compgen -A noaction; echo "st $?"
compgen -o nooption; echo "st $?"
compgen -W 'aa ab' a b c
compgen -W 'a a b' -W 'c'
unalias -a; alias zz=1 fee=one fi=two fo=three fum=four
zfun() { :; }; touch x.c
compgen -a -A function -W zw -G 'x*' z
compgen -a -X 'fo*' f
compgen -k | tr '\n' ' '; echo
compgen -A setopt | tr '\n' ' '; echo
compgen -A shopt | head -3
compgen -b | tr '\n' ' '; echo
enable -n echo; compgen -b ec; compgen -A disabled; compgen -A enabled ec; enable echo
compgen -A helptopic | head -8 | tr '\n' ' '; echo
compgen -A signal | head -5 | tr '\n' ' '; compgen -A signal | tail -5 | tr '\n' ' '; echo
declare unsetv; compgen -v unsetv; echo "st $?"
declare -A emptyassoc; arr=(1); declare -A as=([k]=v); compgen -A arrayvar | grep -E '^(arr|as|emptyassoc)$'
export zexp=1; compgen -e zex
compgen -A binding | head -3
compgen -W "ab 'c d'" "'c"
compgen -A shopt '"no' | head -2
# -W: IFS splitting, quotes and expansions
x=val; compgen -W '$x "a b" {1,2} $(echo c d) *.c'
IFS=: compgen -W 'a:b c'
IFS= compgen -W 'a b'
compgen -W '${#x} a\ b' | cat -A
# -X, -P and -S
compgen -W 'foo bar baz' -X 'b*'
compgen -W 'foo bar baz' -X '!b*'
compgen -W 'foo fob bar' -X '!&*' fo
compgen -W 'a& b' -X '*\&'
compgen -W 'a b' -X '!'; echo "st $?"
shopt -s extglob; compgen -W 'ab ac ad' -X '!(ab)'; shopt -u extglob
compgen -W 'ab ac ad' -X '!(ab)'; echo "st $?"
shopt -s nocasematch; compgen -W 'Foo foo bar' -X 'f*'; shopt -u nocasematch
compgen -W 'a b' -P '<' -S '>' -X a
# globs, files and directories (sorted: directory order varies)
mkdir d1 d2; touch f1 f2 .hid
compgen -G '*' | sort
compgen -G 'nomatch*'; echo "st $?"
compgen -f f | sort
compgen -d | sort
compgen -d -o plusdirs d | sort
compgen -W zz -o dirnames -P pre q; echo "st $?"
compgen -W dx -o plusdirs -P p d | sort
compgen -W zz -o dirnames d | sort
compgen -o default f | sort
compgen -o bashdefault '$HOM'
compgen -W '${zz?boom} b'
echo not reached
