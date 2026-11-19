# Background jobs that run in a forked child shell, which pins its
# working directory again after taking stock of the descriptors it was
# given: the programs it starts change into that pin. Taken before,
# the pin went stale on macOS, and every such program failed with
# "Invalid argument".
echo hello >f
{ cat f; } & wait
cat <f & wait
{ cat; } <f & wait
{ cat f | tr a-z A-Z; } & wait
( cd sub 2>/dev/null; cat f ) & wait
top=$PWD
p=$(type -P pwd)
mkdir gone && cd gone && rmdir "$top/gone"
{ "$p"; } & wait
"$p" </dev/null & wait
cd "$top"
