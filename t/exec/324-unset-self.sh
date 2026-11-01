once() { unset -f once; echo "once ran"; }
once
once
echo "status $?"
redef() { redef() { echo "new"; }; echo "old"; redef; }
redef
redef
outer() { unset -f helper; helper 2>/dev/null; echo "helper status $?"; }
helper() { echo helper; }
helper
outer
