# bench/agent/driver.sh: runs inside tools/sandbox under busybox sh; see
# bench/agent/run. usage: driver.sh setup|prep|MODE
#
# Environment: BENCH (this directory), SHELLEXE (the shell timed), U and
# M (Git's usr/bin and mingw64/bin), REPO (the scratch repository), and
# REPOU, SNAPU, CWDU, TMPW (paths as the wrapper spells them).
set -u
work=$(pwd)

case $1 in
setup)
    mkdir -p "$REPO/src" || exit 1
    cd "$REPO" || exit 1
    printf '# Demo\n\nA scratch project for the agent batch.\n' >README.md
    i=0
    while [ $i -lt 40 ]; do
        echo "foo line $i bar"
        i=$((i+1))
    done >notes.txt
    {
        echo '#include <stdio.h>'
        echo '#include "util.h"'
        echo
        echo '// TODO: arguments'
        echo 'static int twice(int x) { return 2*x; }'
        echo
        echo 'int main(void)'
        echo '{'
        echo '    printf("%d\n", twice(21));'
        echo '    return 0;'
        echo '}'
    } >src/main.c
    printf 'static int helper;\n// TODO: more\nint util(void) { return helper; }\n' >src/util.c
    printf 'int util(void);\n' >src/util.h
    printf 'a\nb\nc\n' >src/a.txt
    printf 'a\nB\nc\n' >src/b.txt
    g="$M/git.exe -c user.name=bench -c user.email=bench@example.invalid"
    $g init -q && $g add . && $g commit -qm initial || exit 1
    echo changed >>notes.txt
    exit 0
    ;;
prep)
    # Wrapped commands and externals, one per line, before any timing
    sed 's/ ###.*//' "$BENCH/batch.txt" | sed "s|@REPO@|$REPOU|g" |
        sed "s/'/'\"'\"'/g" | while IFS= read -r c; do
        printf '%s\n' "source $SNAPU 2>/dev/null || true && export TEMP='$TMPW' TMP='$TMPW' && shopt -u extglob 2>/dev/null || true && { \\builtin unalias -- 'unsetenv'; \\builtin unset -f -- 'unsetenv'; } >/dev/null 2>&1 || true && eval '$c' < /dev/null && pwd -P >| $CWDU"
    done >"$work/wrapped.txt"
    sed -n 's/^.*### *//p' "$BENCH/batch.txt" >"$work/ext.txt"
    exit 0
    ;;
esac

mode=$1
cd "$REPO" || exit 1
# busybox exports PWD as C:/x, and bash would keep that spelling;
# Claude Code's environment has no PWD
unset PWD OLDPWD
start=$(date +%s%N)
case $mode in
full|noop)
    while IFS= read -r w; do
        [ $mode = noop ] && w=${w%%eval*}"eval ':' < /dev/null && pwd -P >| $CWDU"
        "$SHELLEXE" -c "$w" </dev/null
    done <"$work/wrapped.txt" >"$work/out.$mode" 2>&1
    ;;
exit)
    while IFS= read -r w; do
        "$SHELLEXE" -c exit </dev/null
    done <"$work/wrapped.txt" >"$work/out.exit" 2>&1
    ;;
ext)
    while IFS= read -r e; do
        eval "$e" </dev/null
        cd "$REPO"
    done <"$work/ext.txt" >"$work/out.ext" 2>&1
    ;;
empty)
    while IFS= read -r e; do
        cd "$REPO"
    done <"$work/ext.txt"
    ;;
esac
end=$(date +%s%N)
echo $(( (end - start)/1000000 ))
