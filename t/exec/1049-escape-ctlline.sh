# An error caught in a program's redirection after a for loop in
# ${ cmd; } there changed the line that a compound command's
# redirection errors cite: the line is as it was, at the top level and
# in a function, as if the redirection had failed in bash's child
{
cat </dev/null >${ for i in 1
do
: ${u?}; done; }
{ :; } </nonexistent
}
f() {
  cat </dev/null >${ for i in 1
  do
  : ${u?}; done; }
  { :; } </nonexistent2
}
f
echo done
