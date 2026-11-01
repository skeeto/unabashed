# In pathname expansion a slash ends a bracket expression's chance: a
# bracket holding a slash is matched literally, component by component.
cd /tmp && mkdir bs && cd bs
mkdir -p 'a[b' a/c ab
touch 'a[b/c]' a/c/d ab/c
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show 'a[b/c]'    a[b/c]
show 'a[/]c'     a[/]c
show 'a[b/c]*'   a[b/c]*
show 'a[bc]/*'   a[bc]/*
show 'a?/*'      a?/*
show '*[b/c]'    *[b/c]
show 'a[!x]/c'   a[!x]/c
GLOBIGNORE='a[/]c'
show 'ignore a[/]c' a/*
GLOBIGNORE='a[a-z]c'
show 'ignore a[a-z]c' a/*
GLOBIGNORE='a?c'
show 'ignore a?c' a/*
GLOBIGNORE='a*'
show 'ignore a*' a/* ab/*
