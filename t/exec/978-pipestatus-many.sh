# PIPESTATUS keeps every stage of a pipeline, as bash has no limit:
# past the few the shell holds itself, the statuses are kept apart, and
# a subshell, a trap or a background job leaves the caller's as they
# were. (300 stages, which once overwrote the shell's state, need more
# memory than the vos gives a test.)
p=false
for ((i = 1; i < 40; i++)); do p+=' | false'; done
eval "$p"'
echo "${#PIPESTATUS[@]} ${PIPESTATUS[0]} ${PIPESTATUS[39]}"'
for i in 1 2; do echo "loop $i"; done
q='(exit 0)'
for ((i = 1; i < 40; i++)); do q+=" | (exit $((i % 7)))"; done
eval "$q"'
ps=("${PIPESTATUS[@]}"); echo "${#ps[@]} ${ps[31]} ${ps[32]} ${ps[39]}"
echo "${ps[*]}"'
eval "$q"'
echo "$(echo "${#PIPESTATUS[@]} ${PIPESTATUS[38]}")"'
# a subshell's own long pipeline leaves the caller's as it was
eval "$q"'
case $(eval "$p"; echo "${#PIPESTATUS[@]}") in *) ;; esac
echo "${#PIPESTATUS[@]} ${PIPESTATUS[39]}"'
eval "$p"'
case $(eval "$q"'"'"'
echo "sub ${#PIPESTATUS[@]} ${PIPESTATUS[39]}"'"'"' >&2) in *) ;; esac
echo "${#PIPESTATUS[@]} ${PIPESTATUS[39]}"'
eval "$q"'
{ echo "job ${#PIPESTATUS[@]} ${PIPESTATUS[39]}"; } & wait'
# so does a trap's, short or long
trap 'true | false | true' DEBUG
eval "$q"'
echo "after a trap: ${#PIPESTATUS[@]} ${PIPESTATUS[39]}"'
trap "$p" DEBUG
eval "$q"'
echo "after a trap: ${#PIPESTATUS[@]} ${PIPESTATUS[39]}"'
true | false | true
echo "after a trap: ${#PIPESTATUS[@]} ${PIPESTATUS[*]}"
trap - DEBUG
eval "${q% | *}"'
echo "${#PIPESTATUS[@]} ${PIPESTATUS[38]}"'
true | false; echo "${#PIPESTATUS[@]} ${PIPESTATUS[*]}"
