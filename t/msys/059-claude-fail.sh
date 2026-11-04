# shell: -c "source $DU/test.sh 2>/dev/null || true && export TEMP='$DW\\tmp' TMP='$DW\\tmp' && shopt -u extglob 2>/dev/null || true && { \\builtin unalias -- 'unsetenv'; \\builtin unset -f -- 'unsetenv'; } >/dev/null 2>&1 || true && eval 'echo start; cd nosuch && echo never' < /dev/null && pwd -P >| $DU/tmp/claude-cwd"
# Claude Code's wrapper around a failing command: the status comes from
# the command, the cwd file is not written, errors name the shell
here=$PWD
trap 'echo "exit $?"; cat "$here/tmp/claude-cwd" 2>/dev/null || echo "no cwd file"' EXIT
