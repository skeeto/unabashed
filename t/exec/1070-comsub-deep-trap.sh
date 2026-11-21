# A trap action runs 80 command substitutions deep, where an ERR
# trap inherited under set -E reports the failing command
set -E
trap 'echo "err at $1: $BASH_COMMAND"' ERR
r() { if (($1 < 80)); then echo $(r $(($1+1))); else false $1; fi; }
r 0
echo "after $?"
