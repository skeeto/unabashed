# vos
# Identity variables an agent's snapshot tests
echo "$OSTYPE"
echo "$MACHTYPE"
echo "$HOSTTYPE"
echo "${BASH_VERSINFO[5]}"
case $OSTYPE in msys|cygwin|win32) echo windows ;; *) echo other ;; esac
echo "$BASH"
echo "$0"
echo "${SHELL-unset}"
