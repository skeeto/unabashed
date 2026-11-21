# A coprocess's descriptor closes and its variables are set again,
# through a nameref whose subscript runs a command substitution
coproc { sleep 0.2; }
declare -n COPROC_PID='a[$(echo 7)]'
exec 60>&-
echo "${COPROC[@]} ${!a[*]}"
declare -n COPROC_PID='b[$(echo 8)]'
exec 63<&-
echo "${COPROC[@]} ${!a[*]} ${!b[*]}"
wait
echo survived
