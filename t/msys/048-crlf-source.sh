# vos
# Every CR in parser input is dropped: a CRLF file sources cleanly
printf 'x=a\r\necho "[$x]"\r\nif true; then\r\n  echo "[y]"\r\nfi\r\n' >c.sh
printf 'f() {\r\n  echo "[f $1]"\r\n}\r\ncase $x in\r\na) echo "[case]";;\r\nesac\r\n' >>c.sh
printf 'cat <<EOF\r\nhere\r\nEOF\r\necho "[q\r]" ${#x}\r\nz=a\rb; echo ${#z}\r\n' >>c.sh
. ./c.sh
f arg
eval "$(printf 'echo "[eval\r]"\r\n')"
