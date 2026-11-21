# ERR after a compound command's redirections fail shows bash's
# line_number_for_err_trap: the line of the last simple command, ( ),
# (( )), [[ ]] or function definition run in this shell (for a pipeline
# or &, set before forking), put back after calls and after eval,
# source or ${ cmd; }; a ( list )'s redirection errors and its ERR cite
# its closing line
printf '\n\n:\n' >e7s.sh
trap 'echo ERR $LINENO' ERR
echo one
{ :; } </nonexistent/1
x=1

{ :; } </nonexistent/2
for i in 1; do
  :
done
{ :; } </nonexistent/3
for i in 1; do
  { :; } </nonexistent/4
done
f() {
  echo in f
  { :; } </nonexistent/5
}
f
cat </nonexistent/6
(( 1 ))
{ :; } </nonexistent/7
[[ 1 ]]
{ :; } </nonexistent/8
case a in a)
  { :; } </nonexistent/9
esac
while :; do break; done </nonexistent/10
if :; then :; fi </nonexistent/11
( : ) </nonexistent/12
echo ${ :
:; } >/dev/null
{ :; } </nonexistent/13
true |
false
{ :; } </nonexistent/14
eval $'\n\n:'
{ :; } </nonexistent/15
for i in 1; do
  true |
  true
  { :; } </nonexistent/1
done
g() { :; }
{ :; } </nonexistent/2
h() {
  :
}
{ :; } </nonexistent/3
h
{ :; } </nonexistent/4
( :
)
{ :; } </nonexistent/5
x=${ :
:; }
{ :; } </nonexistent/6
source ./e7s.sh
{ :; } </nonexistent/7
for i in 1; do
  true | true
done
{ :; } </nonexistent/8
k() { false; }
k
{ :; } </nonexistent/9
trap 'echo DEBUG >/dev/null' DEBUG
{ :; } </nonexistent/10
trap - DEBUG
if
  false
then :; fi
{ :; } </nonexistent/11
: &
{ :; } </nonexistent/12
coproc { :; }
wait
{ :; } </nonexistent/13
for ((i=0;i<1;i++)); do :; done
{ :; } </nonexistent/14
select s in; do :; done
{ :; } </nonexistent/15
! :
{ :; } </nonexistent/17
{ :; } </nonexistent/18 || :
f2() {
  { :; } </nonexistent/19
}
set -E
f2
set +E
false
{ :; } </nonexistent/20
( :
) </nonexistent/1
for i in 1; do
  ( :
  ) </nonexistent/2
  ( :
  false
  )
  ( echo $LINENO
  )
  ( :
  )
  { :; } </nonexistent/3
done
( :
:
) | cat
{ :; } </nonexistent/4
x=1
( :
:
) &
{ :; } </nonexistent/5
set -E
f() {
  echo a
  for i in 1; do { :; } </nonexistent/1; done
  g
  { :; } </nonexistent/2
}
g() {
  :
}
f
shopt -s lastpipe
echo x |
  read v
{ :; } </nonexistent/3
shopt -u lastpipe
{ echo; } |
{ cat; } >/dev/null
{ :; } </nonexistent/4
true | true &
wait
{ :; } </nonexistent/5
trap 'echo D >/dev/null' DEBUG
echo dbg
{ :; } </nonexistent/6
trap - DEBUG
(( 0 ))
{ :; } </nonexistent/7
[[ -z x ]]
{ :; } </nonexistent/8
x=$(false)
{ :; } </nonexistent/9
eval 'false
:'
{ :; } </nonexistent/10
until :; do :; done </nonexistent/11
h() { { :; } </nonexistent/12; }
h
k() ( :
)
k
{ :; } </nonexistent/13
