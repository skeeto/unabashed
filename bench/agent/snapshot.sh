# A stand-in for Claude Code's shell snapshot, which its Bash tool
# sources before every command: the same kinds of constructs, smaller.
unalias -a 2>/dev/null
shopt -s expand_aliases
alias ll='ls -l'
alias la='ls -A'
function rg {
    local -a args=(${1+"$@"})
    if [[ -x /usr/bin/rg ]]; then
        ARGV0=rg command rg "${args[@]}"
    else
        command grep -r "${args[@]}"
    fi
}
gitroot() { git rev-parse --show-toplevel 2>/dev/null; }
case $OSTYPE in
msys|cygwin|win32) export CLAUDE_WIN=1 ;;
esac
export PATH="$PATH"
