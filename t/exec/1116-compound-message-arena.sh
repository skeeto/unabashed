# A compound assignment's messages are built in the arena the reporter
# copies: the readonly one extends the name cloned at its tip, so it
# starts below the reporter's copy (it printed garbled). The others
# here start at the copy or are built by the reporter itself.
readonly ro=1
ro=(z); echo notreached
f() { ro=(z y); echo notreached; }; f
a=([9223372036854775807]=x y); echo notreached
declare -p a
declare -A h=('' v k w); echo "declare assoc $?"; declare -p h
b=([@]=x); echo notreached
b=([]=x); echo notreached
complete -p nosuch; echo "complete -p $?"
compopt -Z; echo "compopt -Z $?"
compopt -o; echo "compopt -o $?"
compopt -o bogus; echo "compopt bogus $?"
compopt -o nospace nosuch; echo "compopt nosuch $?"
compgen -V notarr -W 'a b' x; echo "compgen -V $?"
complete -Z; echo "complete -Z $?"
bind -Z; echo "bind -Z $?"
bind -m bogus; echo "bind -m $?"
bind -q bogus; echo "bind -q $?"
bind -u bogus; echo "bind -u $?"
echo end
