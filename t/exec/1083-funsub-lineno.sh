# The current line goes back to the command's own when a ${ cmd; } or
# ${| cmd; } body ends, for LINENO in later words, the ERR trap and
# error messages, nested, in a function, in a for list, a case word, a
# subscript and a builtin's redirection word; and the body is no call
# in FUNCNAME, BASH_LINENO or for caller
trap 'echo ERR at $LINENO' ERR
false ${ echo a
  echo b; } $LINENO
echo next $LINENO
echo ${ echo
  echo; } ${|
  REPLY=1; } $LINENO
echo $LINENO ${ echo ${ :
  : ; }
  : ; } $LINENO
for i in ${ :
  echo 1; } $LINENO; do echo for $i; done
case ${ :
 echo x; } in x) echo case $LINENO;; esac
a=(1 2 3); echo ${a[${ :
 echo 1; }]} $LINENO
f() { echo f ${BASH_LINENO[0]} $LINENO; caller; }
echo ${ :
 f; } $LINENO
g() {
  echo g $LINENO ${ :
  :
  : ; } $LINENO
  false ${ :
  : ; }
}
g
k() { declare -p FUNCNAME BASH_LINENO; }
echo ${ k; }
h() { echo ${ k; }; echo ${ declare -p FUNCNAME; }; }
h
echo ${ declare -p FUNCNAME BASH_LINENO; }
echo ${ eval $'\n\n:'; } $LINENO
echo hi >${ :
echo /nonexistent/x; }
echo $LINENO
: ${ :
:; } ${nosuch?err}
