# bash sets PIPESTATUS from an (( )) or [[ ]] command's status after a
# ! inverts it; for other commands, the status before.
! ((z++)); echo "${PIPESTATUS[*]} $?"
! ((1)); echo "${PIPESTATUS[*]} $?"
! ((08)) 2>/dev/null; echo "${PIPESTATUS[*]} $?"
! [[ a == b ]]; echo "${PIPESTATUS[*]} $?"
! [[ a ]]; echo "${PIPESTATUS[*]} $?"
! false; echo "${PIPESTATUS[*]} $?"
! (false); echo "${PIPESTATUS[*]} $?"
! { true; }; echo "${PIPESTATUS[*]} $?"
((0)); echo "${PIPESTATUS[*]} $?"
[[ -z a ]]; echo "${PIPESTATUS[*]} $?"
