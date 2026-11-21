# The setlocale warning of a hook run as a temporary binding goes, or as
# a function's locals go, cites the line bash has then: it puts its line
# back before it drops the temporary environment, to that of the
# enclosing call, for, case or top-level command, and before it pops a
# function's locals, to the caller's line. Defined through eval or not.
exec 2>&1
export LC_ALL=x0
eval 'f() {
  local LC_ALL=x1
  :
}'
f
eval '
g() {
  export LC_ALL=x2
  LC_ALL=x3 :
  echo "lineno $LINENO"
}'
g
h() {
  LC_ALL=x4 true
  for i in 1; do
    LC_ALL=x5 :
  done
  case x in
    x) LC_ALL=x6 :
  esac
  [[ 1 ]] && LC_ALL=x7 :
  k
}
k() {
  LC_ALL=x8 :
}
h
{
  LC_ALL=x9 :
}
LC_ALL=y1 h
echo end
