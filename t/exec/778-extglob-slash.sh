# With extglob, a slash inside a group does not split the pattern into
# directory and name, and so it never matches.
shopt -s extglob
cd /tmp && mkdir es && cd es
mkdir -p a/b c
touch a/b/f c/g
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '@(a|c)/*'     @(a|c)/*
show '@(a/b|c)'     @(a/b|c)
show '@(a/b|c)/*'   @(a/b|c)/*
show '*/@(b|g)'     */@(b|g)
show '!(c)/*/f'     !(c)/*/f
show '+(a|c)/@(b/f)' +(a|c)/@(b/f)
show '@(a|b)/b/f'   @(a|b)/b/f
