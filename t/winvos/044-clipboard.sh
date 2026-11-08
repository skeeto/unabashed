# personality: msys
# Hand-checked against Git Bash 5.3.15 (notes/research/msys2.md): reading
# /dev/clipboard gives the Windows clipboard's text as it is when opened,
# CRLF kept, and nothing (not an error) when it holds no text.
# Where another program keeps it open, Git Bash waits half a second
# and reads nothing, but unabashed retries briefly and then fails.
read -r x </dev/clipboard; echo "empty $? [$x]"
printf 'one\r\ntwo\n' | clipctl set
od -c </dev/clipboard
x=$(</dev/clipboard); printf %s "$x" | od -c
IFS= read -r x </dev/clipboard; printf %s "$x" | od -c
cat </dev/clipboard | wc -c
exec 3</dev/clipboard
printf new | clipctl set
od -c <&3; exec 3<&-
cat </dev/clipboard; echo
[ -c /dev/clipboard ] && [ -r /dev/clipboard ] && [ ! -f /dev/clipboard ] && echo "a character device"
printf 'caf\303\251' | clipctl set
od -c </dev/clipboard
printf 'echo sourced $#' | clipctl set
. /dev/clipboard a b
clipctl nontext
cat </dev/clipboard; echo "image $?"
x=$(</dev/clipboard); echo "image [$x] $?"
printf kept | clipctl set
clipctl busy 2
cat </dev/clipboard; echo "busy $?"
x=$(</dev/clipboard); echo "busy [$x] $?"
cat </dev/clipboard; echo " free $?"
clipctl count
