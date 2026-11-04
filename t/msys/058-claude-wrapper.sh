# shell: -c "source $DU/test.sh 2>/dev/null || true && export TEMP='$DW\\tmp' TMP='$DW\\tmp' && shopt -u extglob 2>/dev/null || true && { \\builtin unalias -- 'unsetenv'; \\builtin unset -f -- 'unsetenv'; } >/dev/null 2>&1 || true && eval 'cd sub && echo '\"'\"'in sub'\"'\"' && pwd && echo \"\$OSTYPE \$TEMP\" && argv /c/x '\"'\"'C:\\x'\"'\"'' < /dev/null && pwd -P >| $DU/tmp/claude-cwd"
# Claude Code's Bash tool wrapper (notes/research/msys2.md section 7),
# byte for byte but for the paths; this file stands in for the shell
# snapshot it sources first. The cwd file is shown on exit.
mkdir -p sub
here=$PWD
trap 'cat "$here/tmp/claude-cwd"' EXIT
echo "snapshot $-"
