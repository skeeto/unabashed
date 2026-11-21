# fc -s runs a copy of the entry: putting the command in the history in
# fc's place, with erasedups, removes the entry it was taken from
set -o history
HISTCONTROL=erasedups
echo hi there
fc -s
echo second entry here
fc -s second=other
HISTCONTROL=
echo third
fc -s third
history | sed 's/^ *[0-9]* *//'
