# Unreadable directories cannot be listed and unsearchable ones cannot
# be entered; pathname expansion then finds nothing there.
cd /tmp && mkdir pm && cd pm
mkdir r x rx
touch r/a x/b rx/c
chmod 444 r; chmod 111 x; chmod 555 rx
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '*/*'     */*
show '*/.'     */.
show '*/'      */
show 'r/*'     r/*
show 'x/*'     x/*
show 'x/b'     x/b*
show '*/b'     */b
show '*/a'     */a
show 'rx/*'    rx/*
chmod 755 r x rx
