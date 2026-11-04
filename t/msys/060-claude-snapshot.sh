# shell: -c "source $DU/test.sh 2>/dev/null || true && export TEMP='$DW\\tmp' TMP='$DW\\tmp' && shopt -u extglob 2>/dev/null || true && { \\builtin unalias -- 'unsetenv'; \\builtin unset -f -- 'unsetenv'; } >/dev/null 2>&1 || true && eval 'll; rg -n pat file; greet '\"'\"'a b'\"'\"' c; echo \"\${#arr[@]} \$(type -t rg)\"' < /dev/null && pwd -P >| $DU/tmp/claude-cwd"
# The constructs of Claude Code's shell snapshot, as the wrapper sources it
unalias -a
shopt -s expand_aliases
alias ll='echo ll-alias'
function rg {
    local -a args=(${1+"$@"})
    [[ -x /usr/bin/ls ]] && echo "rg ${#args[@]}: ${args[*]}"
}
greet() { printf '[%s]\n' "$@"; }
arr=(x y z)
echo "${arr[@]+"${arr[@]}"}"
hash ls 2>/dev/null
typeset -f greet
export PATH="$PATH:/c/nonexistent"
(ARGV0=rg exec -a rg argv 1) | sed -n 2p
