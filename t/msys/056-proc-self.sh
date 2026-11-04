# /proc as an agent's snapshot reads it (no pids printed). This MSYS2
# runtime has no /proc/PID/comm: the snapshot's read of it fails quietly.
[ -d /proc/self ] && echo self
[ -d "/proc/$$" ] && echo pid
cat /proc/self/comm 2>/dev/null || echo "no comm"
cat "/proc/$$/comm" 2>/dev/null || echo "no comm"
{ read -r comm <"/proc/$$/comm"; } 2>/dev/null || echo "no comm"
read -r exe <"/proc/$$/exename"; echo "$exe"
read -r exe <"/proc/$$/winexename"; echo "$exe"
[ -e /proc/cygdrive ] && echo cygdrive
[ -e /PROC ] || echo case-sensitive
[ -e /proc/self/fd/0 ] && echo fd0
readlink /proc/self/exe
