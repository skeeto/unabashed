# Record pattern matching results from bash 5.3 for t/unit/pattern_test.
# See pattern_record.sh for running it.
#
# Reads test inputs on stdin, one per line, tab-separated, and writes
# each line back with bash's result appended as a last field (comment
# lines starting with # pass through):
#
#   m FLAGS PAT STR        whole match: case "$STR" in $PAT)
#   f FLAGS OP PAT STR     ${STR OP PAT} for OP in # ## % %%, or with a
#                          replacement of <> for OP in / // /# /%
#   g FLAGS PAT NAME       does pathname expansion of PAT select NAME?
#   h FLAGS PAT            is PAT subject to pathname expansion?
#   i FLAGS PAT PATH       does GLOBIGNORE=PAT remove the file PATH (no
#                          empty, . or .. components) from */*...?
#
# FLAGS letters: e extglob, i nocasematch/nocaseglob, u LC_ALL=C.UTF-8,
# D dotglob, S no globskipdots, - none. Fields use %HH for bytes outside
# ! to ~ and for %. Results are 0/1, or the encoded expansion for f.
#
# This is free and unencumbered software released into the public domain.

dec() {
    local LC_ALL=C x=${1//\\/\\\\}
    x=${x//[%]/\\x}
    printf -v R '%b' "$x"
}

enc() {
    local LC_ALL=C s=$1 o= c i
    for ((i = 0; i < ${#s}; i++)); do
        c=${s:i:1}
        case $c in
        [!!-~]|%) printf -v c '%%%02X' "'$c" ;;
        esac
        o+=$c
    done
    R=$o
}

setflags() {
    shopt -u extglob nocasematch nocaseglob dotglob nullglob
    shopt -s globskipdots
    LC_ALL=C
    case $1 in *e*) shopt -s extglob ;; esac
    case $1 in *i*) shopt -s nocasematch nocaseglob ;; esac
    case $1 in *u*) LC_ALL=C.UTF-8 ;; esac
    case $1 in *D*) shopt -s dotglob ;; esac
    case $1 in *S*) shopt -u globskipdots ;; esac
}

# Each g line globs in a directory of its own holding just its name, so
# that slow patterns are not multiplied by the number of names.
top=$(mktemp -d)
mkdir "$top/empty"
ng=0
while IFS= read -r line; do
    case $line in
    '#'*|'') printf '%s\n' "$line"; continue ;;
    esac
    IFS=$'\t' read -r kind flags a b c <<<"$line"
    LC_ALL=C
    case $kind in
    m)
        dec "$a"; p=$R
        dec "$b"; s=$R
        setflags "$flags"
        case $s in $p) r=1 ;; *) r=0 ;; esac
        if [[ $flags == *e* ]]; then
            # [[ == ]] always matches with extglob; check that they agree
            if [[ $s == $p ]]; then x=1; else x=0; fi
            [[ $x == $r ]] || echo "warning: case and [[ differ: $line" >&2
        fi
        ;;
    f)
        dec "$b"; p=$R
        dec "$c"; s=$R
        setflags "$flags"
        case $a in
        '#')   x=${s#$p} ;;
        '##')  x=${s##$p} ;;
        '%')   x=${s%$p} ;;
        '%%')  x=${s%%$p} ;;
        '/')   x=${s/$p/<>} ;;
        '//')  x=${s//$p/<>} ;;
        '/#')  x=${s/#$p/<>} ;;
        '/%')  x=${s/%$p/<>} ;;
        esac
        LC_ALL=C
        enc "$x"; r=$R
        ;;
    g)
        dec "$a"; p=$R
        dec "$b"; n=$R
        mkdir "$top/g$((++ng))"
        [[ $n == . || $n == .. ]] || : >"$top/g$ng/$n"
        cd "$top/g$ng"
        setflags "$flags"
        shopt -s nullglob
        shopt -u nocasematch  # for comparing the results
        r=0
        for f in $p; do
            [[ $f == "$n" ]] && r=1
        done
        shopt -u nullglob
        cd /
        ;;
    h)
        dec "$a"; p=$R
        cd "$top/empty"
        setflags "$flags"
        shopt -s nullglob
        IFS=
        set -- $p
        IFS=$' \t\n'
        r=$(( $# == 0 ))
        shopt -u nullglob
        cd /
        ;;
    i)
        dec "$a"; p=$R
        dec "$b"; n=$R
        mkdir "$top/i$((++ng))"
        cd "$top/i$ng"
        g='*'
        d=$n
        while [[ $d == */* ]]; do d=${d%/*}; g+='/*'; done
        [[ $n == */* ]] && mkdir -p -- "${n%/*}"
        : >"$n"
        setflags "$flags"
        shopt -u nocasematch
        GLOBIGNORE=$p
        r=1
        for f in $g; do
            [[ $f == "$n" ]] && r=0
        done
        unset GLOBIGNORE
        cd /
        ;;
    *)
        r=?
        ;;
    esac
    LC_ALL=C
    printf '%s\t%s\n' "$line" "$r"
done
rm -rf "$top"
