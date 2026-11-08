# complete and compopt: defining, listing in bash's hash order, quoting,
# removing, and the errors
complete
complete -F _f -o filenames -o nospace foo 'b ar' ''
complete -D -W d; complete -E -W e; complete -I -W i
complete -p
complete -r
complete a; echo "st $?"
complete -W 1 a b c d e; complete -p
complete -W 2 c; complete -r b; complete -W 3 b; complete -p
complete -r
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
complete -W "it's" -E
complete -o bashdefault -o default -o dirnames -o fullquote -o noquote -o nosort -o plusdirs -b -c -d -e -f -j -k -s -u -A arrayvar -A binding -A disabled -A enabled -A function -A helptopic -A running -A setopt -A shopt -A stopped x
complete -W a 'x$y' '~a' a~ '#a' a# \' ''
complete -F 'f$x' y
complete -W "'" q
complete -p foo -E x 'x$y' '~a' a~ '#a' a# \' '' y q
complete -p nope foo; echo "st $?"
complete -r nope; echo "st $?"
complete -r -I; echo "st $?"
complete -p -D -E; echo "st $?"
complete -D -W x foo; complete -p foo
complete -o nospace; echo "st $?"
complete -A bogus x; echo "st $?"
complete -o bogus x; echo "st $?"
complete -z; echo "st $?"
complete -A; echo "st $?"
complete -F 'a b' x; echo "st $?"
complete -V v x; echo "st $?"
complete +o nospace q; echo "st $?"
complete -r -W x foo; complete -p foo; echo "st $?"
# one complete command's names share a compspec
complete -r
complete -W x -o filenames sa sb
compopt -o nospace sa; complete -p sb
complete -W y sa; compopt +o filenames sb; complete -p sa sb
compopt sa sb
compopt -o nospace +o nospace -o nosort sa; compopt sa
compopt nope; echo "st $?"
compopt -D; echo "st $?"
compopt; echo "st $?"
compopt -o nospace; echo "st $?"
compopt -p; echo "st $?"
compopt -o bogus sa; echo "st $?"
compopt -o; echo "st $?"
complete -D -W x; compopt -D; compopt -o default -D; compopt -D
( complete -r; complete -p ); complete -p | wc -l
