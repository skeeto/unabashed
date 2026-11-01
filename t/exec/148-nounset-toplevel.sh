# set -u error at top level exits the shell
set -u
echo before
echo "$undefined_var"
echo not reached
