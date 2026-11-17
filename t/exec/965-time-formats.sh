# TIMEFORMAT's %E (bash's undocumented alias of %R) and %P, which bash
# takes only right after the %; a bare time in POSIX mode reports the
# shell's own user and system time so far. times prints the shell's
# times and then its children's. Digits are masked and runs of them
# squeezed, as times vary.
mask() { tr 0-9 D | tr -s D; }
for f in '%E' '%3lE|%0E' '%P' 'x%Py' '%3P' '%lP' '%%P' '%P%'; do
    TIMEFORMAT=$f
    { time :; } 2>&1 | mask
done
unset TIMEFORMAT
times | mask
sleep 0 & wait
times | mask
set -o posix
{ time; } 2>&1 | mask
{ time :; } 2>&1 | mask
{ time >/dev/null; } 2>&1 | mask
{ time x=1; } 2>&1 | mask
TIMEFORMAT='[%R %lU]'
{ time; } 2>&1 | mask
