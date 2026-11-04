# vos
# Command substitution strips trailing \r\n pairs, keeps inner CRs;
# read -r keeps the CR
show() { local s=$1; s=${s//$'\r'/'\r'}; s=${s//$'\n'/'\n'}; echo "[$s] ${#1}"; }
show "$(printf 'a\r\n')"
show "$(printf 'a\rb\r\nc\r\n\r\n')"
show "$(printf 'a\n\r\n')"
show "$(printf 'a\r\r\n')"
show "$(printf 'a\r')"
show "$(printf '\r\n\r\n')"
printf 'b\r\n' >f
read -r line <f
show "$line"
mapfile -t arr <f
show "${arr[0]}"
show "$(<f)"
