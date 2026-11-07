# A compound command's redirection errors cite bash's line_number:
# where the parser finished the top-level command, or the line of the
# enclosing for, case, select or function call, which simple commands
# never change and those restore when done
exec 2>&1
{ echo a
  while false; do :; done > /nonexistent/a
  echo b
}
( trap 'echo got ERR' ERR
  echo c
  while false; do :; done > /nonexistent/b
  echo after $?
)
{ echo d
  { echo e; } > /nonexistent/c
  echo f
  cat /nonexistent/g
  if true; then :; fi > /nonexistent/d
}
f() {
  echo in f
  while false; do :; done > /nonexistent/e
  for i in 1; do :; done > /nonexistent/f
  echo out
}
echo between
f
{ for i in 1; do
    echo $i
  done
  while false; do :; done > /nonexistent/h
  case x in x) ;; esac
  while false; do :; done > /nonexistent/i
  echo $LINENO
  while false; do :; done > /nonexistent/j

}
echo end
for i in 1
do
  echo body
  { :; } > /nonexistent/a
  case x in
  x)
    { :; } > /nonexistent/b
    ;;
  esac
  { :; } > /nonexistent/c
done
g() { { :; } > /nonexistent/d; }
f() {
  g
  { :; } > /nonexistent/e
}
{
  f
  { :; } > /nonexistent/f
}
while read -r x
do
  { :; } > /nonexistent/g
done <<EOF
one
EOF
echo $LINENO
