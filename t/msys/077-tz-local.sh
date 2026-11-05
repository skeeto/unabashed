# env: -TZ
# With TZ unset the shell's local time is the system zone's, as the
# MSYS2 runtime gives it to Git's date (whatever this machine's zone is)
same() {
    if [ "$2" = "$3" ]; then echo "$1 same"; else echo "$1: $2 / $3"; fi
}
echo "${TZ-unset}"
f='%F %T %Z %z'
for t in 1700000000 1690000000 946684800 1234567890; do
    same "$t" "$(printf "%($f)T" $t)" "$(date -d @$t "+$f")"
done
p='\D{%Z %z}'
same prompt "${p@P}" "$(date '+%Z %z')"
TZ=JST-9
same unexported "$(printf "%($f)T" 0)" "$(date -d @0 "+$f")"
export TZ
same exported "$(printf "%($f)T" 0)" "$(date -d @0 "+$f")"
