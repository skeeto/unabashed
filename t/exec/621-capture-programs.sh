# Programs writing to a command substitution: in order with the shell's
# own output, from every stage and descriptor that leads there, and the
# substitution ends only when every writer has
x=$(echo a; cat <<<b; echo c); echo "1: ${x//$'\n'/,}"
x=$(printf x; sleep 0; printf y); echo "2: $x"
x=$(ls /nonexistent 2>&1); echo "3: ${x%%:*}"
x=$(exec 2>&1; ls /nonexistent | cat; echo after); echo "4: ${x//$'\n'/,}" | sed 's/:.*,/: ... ,/'
x=$(printf '%s\n' 3 1 2 | sort | tr '\n' ' '); echo "5: $x"
x=$(bash -c 'echo nested; echo err >&2' 2>&1); echo "6: ${x//$'\n'/,}"
x=$(echo one; bash -c 'printf two'; echo; echo three); echo "7: ${x//$'\n'/,}"
x=$(cat <(echo from-procsub) - <<<from-stdin); echo "8: ${x//$'\n'/,}"
x=$(echo out > >(tr a-z A-Z); echo here); echo "9: $(sort <<<"$x" | tr '\n' ,)"
x=$( { echo s1; ls /nonexistent; echo s2; } 2>&1 | wc -l); echo "10: $x"
x=$(yes | head -3 | tr -d '\n'); echo "11: $x"
x=$( (exec 1>&2) 2>&1; echo after-exec); echo "12: $x"
