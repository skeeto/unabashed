# Commands run by eval, source or ${ cmd; } inside a for, case, call or
# group leave the line that a compound command's redirection errors and
# for's own messages cite as it was, as bash's parse_and_execute puts
# its line_number back
printf '\n\n\n:\n' >c1s.sh
for i in 1; do
  eval $'\n\n\n:'
  { :; } </nonexistent/1
done
for i in 1; do
  : ${ :
  :
  :; }
  { :; } </nonexistent/2
done
case x in x)
  source ./c1s.sh
  { :; } </nonexistent/3
esac
for i in 1; do
  eval $'\n\n\n:'
  for j in ${ eval $'\n\n:'; echo 1; }; do eval $'\n:'; done </nonexistent/4
done
for i in 1; do
  : ${ :
  :; }
  for j in 1; do :; done <&-; for k; do :; done; for 1x in a; do :; done
done
for i in 1; do
  eval $'\n\n\n:'
  for 2x in a; do :; done
done
f() {
  eval $'\n\n\n:'
  { :; } </nonexistent/5
}
f
{ eval $'\n\n:'; { :; } </nonexistent/6; }
trap 'eval $'"'"'\n\n\n:'"'"'' DEBUG
for i in 1; do
  :
  { :; } </nonexistent/7
done
trap - DEBUG
echo $?
