# The commands of a trap action set the line ERR goes on from, and
# bash puts it back when the action ends: in a pipeline stage or a
# background job too, a multi-line DEBUG action does not move ERR's
# $LINENO for the command it ran before
trap ':
:
:' DEBUG
set -T -E
trap 'echo ERR $LINENO' ERR
f() {
  ( :
    { :; } </nonexistent/a
    false
  ) | cat
  ( :
    { :; } </nonexistent/b
  ) &
  wait
  : | (
    :
    false
  )
}
f
