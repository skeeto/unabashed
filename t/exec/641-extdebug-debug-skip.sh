# Under extdebug, a DEBUG trap that returns non-zero skips the command;
# 2 in a function also returns from it
shopt -s extdebug
dbg() { [[ $BASH_COMMAND == *skipme* ]] && return 1; [[ $BASH_COMMAND == *leave* ]] && return 2; return 0; }
f() { echo one; echo skipme; echo leave; echo never; }
trap dbg DEBUG
echo start
echo skipme
x=1; x=2 skipme=1
echo "x=$x"
f
echo "status $?"
(( skipme = 5 )); echo "skipme=${skipme:-unset}"
trap - DEBUG
shopt -u extdebug
trap dbg DEBUG
echo skipme without extdebug
trap - DEBUG
