# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 5).
# The environment imported by busybox-w32's rules: names uppercased,
# values with forward slashes except COMSPEC and SYSTEMROOT, HOME from
# USERPROFILE and not exported, an invalid name renamed with _. The
# identity variables of a native shell. The hidden shopt msyspath is
# in no listing; setting it renders PATH and PWD the msys way.
echo "$PATH"
echo "$COMSPEC|$SYSTEMROOT|$TEMP"
echo "$HOME|$USERPROFILE"
declare -p HOME | cut -c1-11
echo "${Path-unset} ${ComSpec-unset}"
echo "$PROGRAMFILES_X86_"
echo "$OSTYPE $MACHTYPE $HOSTTYPE ${BASH_VERSINFO[5]}"
echo "$BASH" "$SHELL"
shopt | grep -c msyspath
shopt -p | grep -c msyspath
case :$BASHOPTS: in *:msyspath:*) echo in BASHOPTS;; esac
shopt -q msyspath
echo "query $?"
shopt msyspath
cd C:/Windows
shopt -s msyspath
echo "$PATH"
echo "$PWD"
pwd -W
shopt msyspath
shopt -p msyspath
shopt | grep -c msyspath
shopt -u msyspath
echo "$PATH"
echo "$PWD"
shopt -s nosuchopt
