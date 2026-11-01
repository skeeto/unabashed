# ${x:?} at top level exits the shell
echo before
: ${unset_one:?gone}
echo not reached
