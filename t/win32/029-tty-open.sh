# conrun: sleep:100
# A descriptor the shell opens on the console is a terminal, by either
# name, for reading or writing; the files it was given are not
[ -t 0 ]; echo "stdin $?"
[ -t 1 ]; echo "stdout $?"
exec 3</dev/tty 4>/dev/tty 5<CONIN\$ 6>CONOUT\$ 7<>/dev/tty
for fd in 3 4 5 6 7; do [ -t $fd ]; echo "fd $fd $?"; done
echo "to the console" >&4; echo "write $?"
ls CONOUT\$ CONIN\$ 2>/dev/null; echo "no files $?"
