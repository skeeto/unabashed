# A program whose descriptors lead to two or more command substitutions
# writes to each of them, in order with the shell's own output there
out=$( { err=$(bash -c 'echo to-stdout; echo to-stderr >&2' 2>&1 1>&3); echo "err=[$err]"; } 3>&1 )
echo "1: out=[$out]"
out=$( { echo before; err=$(bash -c 'echo o1; echo e1 >&2; echo o2' 2>&1 1>&3); echo "err=[$err]"; echo after; } 3>&1 )
echo "2: ${out//$'\n'/,}"
x=${ exec 3>&1; y=${ bash -c 'echo inner; echo outer >&3'; }; echo "y=[$y]"; }
echo "3: ${x//$'\n'/,}"
a=$( { b=$( { c=$(bash -c 'echo to-c; echo to-b >&3; echo to-a >&4'); echo "c=[$c]"; } 3>&1 ); echo "b=[$b]"; } 4>&1 )
echo "4: ${a//$'\n'/,}"
a=$( { b=$( { c=$(cat /nonexistent 2>&4 3>&-; echo "st $?" >&3); echo "c=[$c]"; } 3>&1 ); echo "b=[$b]"; } 4>&1 )
echo "5: ${a//$'\n'/,}"
