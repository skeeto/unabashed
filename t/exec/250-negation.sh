# ! inverts the exit status of a pipeline
! true; echo "! true: $?"
! false; echo "! false: $?"
! (exit 3); echo "! exit 3: $?"
! { false; }; echo "! group: $?"
! true | false; echo "! pipe: $?"
! false | true; echo "! pipe2: $?"
! if false; then :; fi; echo "! if: $?"
! while false; do :; done; echo "! while: $?"
! case x in y) ;; esac; echo "! case: $?"
! [[ a = b ]]; echo "! cond: $?"
! (( 0 )); echo "! arith: $?"
! for i in; do :; done; echo "! for: $?"
f() { return 4; }
! f; echo "! f: $?"
! ! true; echo "!!: $?"
if ! false; then echo "if !"; fi
while ! true; do echo never; done
echo "while !: $?"
