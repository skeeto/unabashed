# A native program's CRLF output through command substitution, and the
# //c escape that keeps cmd's switch a switch
x=$(cmd //c echo hi)
echo "[$x] ${#x}"
x=$(cmd //c 'echo a&echo b')
echo "${x//$'\r'/'\r'}" ${#x}
cmd //c echo hi | od -c
cmd //c exit 3; echo "status $?"
