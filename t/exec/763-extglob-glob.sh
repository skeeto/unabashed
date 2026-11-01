# extglob operators in pathname expansion: leading dots, negation, and
# groups that could match a dot.
shopt -s extglob
cd /tmp && mkdir eg && cd eg
touch a.c b.c c.h d.o .e.c .f Makefile README a.cc
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '*.@(c|h)'   *.@(c|h)
show '!(*.c)'     !(*.c)
show '+([a-d]).c' +([a-d]).c
show '?(a).c'     ?(a).c
show '*(a|b).c'   *(a|b).c
show '@(*.o|M*)'  @(*.o|M*)
show '.!(|.)'     .!(|.)
show '@(.*|*)'    @(.*|*)
show '!(*.?)'     !(*.?)
show '*.!(c|h)'   *.!(c|h)
show '@(a|b).c?(c)' @(a|b).c?(c)
show 'R!(x)'      R!(x)
shopt -s dotglob
show 'dot !(*.c)' !(*.c)
show 'dot *.c'    *.c
shopt -u dotglob
shopt -s nullglob
show 'null +(z)' +(z)
shopt -u nullglob extglob
show 'off' *.c
