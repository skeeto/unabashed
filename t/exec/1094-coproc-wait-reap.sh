# A coprocess that ended is dropped (its variables unset, descriptors
# closed) where bash's cleanup_dead_jobs runs: when wait collects a job,
# the one waited for (by pid or job spec), named or not, or another;
# after a fork (subshell, substitution, pipeline); as a line of eval,
# source, a trap or ${ ...; } is read; and as the next coprocess starts,
# after its pipes are made. A live one keeps its variables, closing its
# descriptors only sets them -1, and a subshell, where they are hidden,
# leaves the parent's open.
p() { echo "-- $1"; declare -p ${2:-COPROC} 2>&1; local v=${2:-COPROC}_PID; echo "$v ${!v+set}"; }
coproc { sleep 0.1; }; wait $COPROC_PID; p "wait pid, same line"
coproc { sleep 0.1; }; wait %%; echo "status $?"; p "wait %%, same line"
coproc A { sleep 0.1; }; coproc B { read; }; wait $A_PID; p "waited A" A; p "B lives" B
exec {B[1]}>&-; p "B input closed" B; wait $B_PID; p "waited B" B
coproc { read; }; exec {COPROC[0]}<&- {COPROC[1]}>&-; p "both closed"; wait $COPROC_PID; p "both closed, waited"
coproc { :; }; sleep 0.3 & wait $!; p "after waiting another job"
coproc { sleep 0.1; }; f() { wait $COPROC_PID; p "in function after wait"; }; f
coproc C { sleep 0.1; }; coproc D { sleep 0.1; }; wait $D_PID $C_PID; p "both waited, C" C; p "both waited, D" D

# D stays alive throughout; reading it with a timeout lets the other end
coproc D { read; echo "D read $REPLY"; }
chk() { { : >&${D[1]} && : <&${D[0]} && echo "D open"; } 2>&1; }
(:); chk
coproc { :; }; read -t 0.3 -u ${D[0]}; p "dead, not noticed"
coproc { :; }; read -t 0.3 -u ${D[0]}; eval 'p "in eval"'
coproc { :; }; read -t 0.3 -u ${D[0]}; (p "subshell"); p "after subshell"; chk
coproc { :; }; read -t 0.3 -u ${D[0]}; x=$(:); p "after comsub"
coproc { :; }; read -t 0.3 -u ${D[0]}; x=${ :; }; p "after funsub"
coproc { :; }; read -t 0.3 -u ${D[0]}; true | true; p "after pipeline"
coproc { :; }; read -t 0.3 -u ${D[0]}; source /dev/null; p "after empty source"
coproc { :; }; read -t 0.3 -u ${D[0]}; trap 'p "in trap"' USR1; kill -USR1 $$; trap - USR1
coproc A { :; }; read -t 0.3 -u ${D[0]}; coproc B { read; }; p "after another" A; p "the other" B
exec {B[1]}>&-
echo hi >&${D[1]}; read -u ${D[0]}; echo "$REPLY"
wait
