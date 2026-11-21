# Beyond 1104: a coprocess's ( list ) keeps the shell's line (bash's
# child for it sets none), while a function whose body is a ( list ),
# a lastpipe stage and $LINENO in a compound redirection's word see the
# ) line, and ERR (set -E) cites it for the list's first command and
# the last simple command's line after that.
f() (
  { :; } </nonexistent/a
  { cat; } <<<"f here $LINENO"
  :
  { :; } </nonexistent/b
)
f
p() {
  (
    { :; } </nonexistent/c
  ) | cat
  cat </dev/null | (
    { :; } </nonexistent/d
    { cat; } <<<"stage here $LINENO"
  )
  (
    { :; } </nonexistent/e
  ) &
  wait
  (
    (
      { :; } </nonexistent/f
    )
    { :; } </nonexistent/g
  )
  coproc (
    { :; } </nonexistent/h
  )
  wait
}
p
for i in 1; do
  ( { :; } </nonexistent/i
  ) | cat
  ( { cat; } <<<"loop here $LINENO"
  )
done
shopt -s lastpipe
q() {
  : | (
    { :; } </nonexistent/j
  )
}
q
trap 'echo ERR $LINENO ${BASH_LINENO[0]}' ERR
set -E
f
f | cat
f &
wait
