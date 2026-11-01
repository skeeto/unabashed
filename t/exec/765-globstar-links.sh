# globstar lists links to directories but does not descend into them,
# except where a pattern names them directly.
shopt -s globstar
cd /tmp && mkdir gl && cd gl
mkdir -p real/sub
touch real/f real/sub/g
ln -s real link
ln -s nowhere dangling
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '**'         **
show '**/'        **/
show '**/g'       **/g
show '**/f'       **/f
show 'link/**'    link/**
show 'link/*'     link/*
show '*/'         */
show '*/f'        */f
show 'd*'         d*
show '**/link'    **/link
show '**/link/'   **/link/
