# PIPESTATUS: every stage of the last pipeline; one status after a
# simple command, (( )), [[ ]] or subshell; untouched by other compound
# commands; recreated after an assignment or unset
p() { echo "$1: ${PIPESTATUS[*]} (${#PIPESTATUS[@]})"; }
false | true | (exit 3); p pipe
true; p true
false; p false
false | true; { false; }; p group
false | true; f(){ return 3; }; f; p func
false | true; x=1; p assign
(exit 4) | true; x=$(exit 3); p assigncomsub
false | true; (exit 4); p subshell
false | true; if false; then :; fi; p if
false | true; if true; then (exit 2) | true; fi; p if2
false | true; [[ a = b ]]; p dbrack
false | true; (( 0 )); p arith
false | true; for i in 1; do false; done; p for
false | true; for i in; do false; done; p forempty
false | true; while false; do :; done; p while
false | true; case a in b) ;; esac; p case
! false | true; p bang
! true; p bang2
false | true; true && false; p andand
false | true; false || (exit 5) | true; p oror
false | true; nosuch 2>/dev/null; p nosuch
false | true; eval 'false | (exit 3)'; p eval
false | true; f(){ (exit 2) | true; }; f; p funcpipe
false | true; g(){ (exit 2) | true; return 4; }; g; p funcpipe2
PIPESTATUS=(9 9); p userassign
false | true; declare -p PIPESTATUS
false | true; echo "${PIPESTATUS[@]}" | cat
false | true; (echo "${PIPESTATUS[@]}")
false | true; echo $(echo "${PIPESTATUS[@]}")
unset PIPESTATUS; false | true; p afterunset
false | true; : > /dev/null; p redir
false | true; > /dev/null; p redironly
false | true; exit 3 | false; p exitpipe
false | true; true | { false; }; p last
false | true; true & p bg
wait
false | true; f(){ :; }; p funcdef
false | true; { :; } > /nonexistent/x; p grpredir
false | true; : > /nonexistent/x; p simpredir
false | true; (( x = 0 )) | true; p arithpipe
false | true; echo $((PIPESTATUS[0] + 10)); p arith2
false | (exit 3); echo "${PIPESTATUS[1]} ${PIPESTATUS}"
PIPESTATUS=(9 8) x=${PIPESTATUS[1]}; echo "x=$x"; p same
false | true; declare -a PIPESTATUS; p declare
false | true; readonly PIPESTATUS 2>/dev/null; false | (exit 2); p ro
f() { false | true; echo "3 ${PIPESTATUS[*]}"; }; f | cat; echo "3b ${PIPESTATUS[*]}"
echo ${PIPESTATUS[@]}
false | true; echo "4 ${PIPESTATUS[@]:1} ${!PIPESTATUS[@]} ${PIPESTATUS[-1]}"
false | true; local x 2>/dev/null; echo "5 ${PIPESTATUS[*]}"
false | true; unset 'PIPESTATUS[0]'; echo "6 ${PIPESTATUS[*]}"
false | true; PIPESTATUS[3]=x; declare -p PIPESTATUS
false | true; for ((i=0; i<1; i++)); do :; done; echo "7 ${PIPESTATUS[*]}"
false | true; for ((i=0; i<0; i++)); do :; done; echo "8 ${PIPESTATUS[*]}"
false | true; select s in; do :; done; echo "9 ${PIPESTATUS[*]}"
false | true; [ 1 = 2 ]; echo "10 ${PIPESTATUS[*]}"
false | true; until true; do :; done; echo "11 ${PIPESTATUS[*]}"
false | true; while (exit 3); do :; done; echo "12 ${PIPESTATUS[*]}"
false | true; case x in x) false | (exit 2) ;; esac; echo "13 ${PIPESTATUS[*]}"
g() { local PIPESTATUS; false | true; echo "14 ${PIPESTATUS[*]}"; }; g; echo "14b ${PIPESTATUS[*]}"
false | true; echo "4 ${PIPESTATUS[@]:1} ${!PIPESTATUS[@]} ${PIPESTATUS[-1]}"
false | (exit 2); echo "${PIPESTATUS[@]@A}"
false | (exit 2); echo "${PIPESTATUS[@]@K}"
false | (exit 2); echo "${PIPESTATUS[@]@Q}"
false | (exit 2); echo "${PIPESTATUS[@]@a}"
false | (exit 2); a=("${PIPESTATUS[@]}"); echo "${a[1]}"
false | (exit 2); echo $(( PIPESTATUS[0] + PIPESTATUS[1] ))
f() { echo "${FUNCNAME[@]:0:1} ${FUNCNAME[*]@A}"; }; g() { f; }; g
