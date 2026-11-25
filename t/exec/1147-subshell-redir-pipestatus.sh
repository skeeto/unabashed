# A ( list ) whose redirection fails sets PIPESTATUS, as bash's child
# fails and the parent waits for it; under ! the child inverts its own
# status (but not a fatal error's 127) and the parent inverts that
# again. Other compound commands leave PIPESTATUS as it was.
cd "$(mktemp -d)"
true | false | true; ( : ) 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; ( : ) 3<$u; echo "${PIPESTATUS[@]} $?"
true | false | true; ( : ) 3<${u?}; echo "${PIPESTATUS[@]} $?"
true | false | true; ! ( : ) 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; ! ( : ) 3<${u?}; echo "${PIPESTATUS[@]} $?"
true | false | true; { :; } 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; ! { :; } 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; if :; then :; fi 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; for i in 1; do :; done 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; (( 1 )) 3</nope/f; echo "${PIPESTATUS[@]} $?"
true | false | true; [[ 1 ]] 3</nope/f; echo "${PIPESTATUS[@]} $?"
trap 'echo "ERR ${PIPESTATUS[@]} $?"' ERR
true | false | true; ( : ) 3</nope/f
true | false | true; ! ( : ) 3</nope/f
