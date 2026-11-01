# Record arith.golden from arith.cases with bash 5.3 (run by arith_record.sh).
#
# Each case line is: FLAGS <tab> EXPR [<tab> SETUP]...
#   FLAGS  "-", or "u" for set -u
#   EXPR   the expression, with \\ \t \n \r \xHH escapes
#   SETUP  NAME=VALUE or NAME[N]=VALUE, or readonly:NAME=VALUE
# Lines that are empty or start with # are ignored.
#
# For each case the expression is evaluated twice in fresh subshells:
# once by $((...)) for the value or error message, once by ((...))
# followed by a dump of every variable the case could have touched.
# Recorder variables start with _R_ so tests must not use that prefix.

_R_esc() {
    local s=$1
    s=${s//\\/\\\\}
    s=${s//$'\n'/\\n}
    s=${s//$'\t'/\\t}
    s=${s//$'\r'/\\r}
    _R_REPLY=$s
}

_R_setup() {
    local _R_it _R_ro _R_nm
    for _R_it in "${_R_items[@]}"; do
        _R_ro=0
        if [[ $_R_it == readonly:* ]]; then
            _R_it=${_R_it#readonly:}
            _R_ro=1
        fi
        _R_nm=${_R_it%%=*}
        _R_val=${_R_it#*=}
        eval "$_R_nm=\$_R_val"
        if [[ $_R_ro == 1 ]]; then
            readonly "${_R_nm%%\[*}"
        fi
    done
    unset _R_val
    if [[ $_R_flags == *u* ]]; then
        set -u
    fi
}

_R_dump() {
    set +u
    local _R_n _R_i _R_k
    echo DUMP
    for _R_n in "${_R_names[@]}"; do
        [[ ${_R_pre[$_R_n]} ]] && continue
        declare -p "$_R_n" >/dev/null 2>&1 || continue
        eval "_R_idx=(\"\${!$_R_n[@]}\")"
        for _R_i in "${_R_idx[@]}"; do
            eval "_R_v=\${$_R_n[$_R_i]}"
            if [[ $_R_i == 0 ]]; then _R_k=$_R_n; else _R_k="$_R_n[$_R_i]"; fi
            _R_esc "$_R_k=$_R_v"
            echo "state $_R_REPLY"
        done
    done | LC_ALL=C sort
}

while IFS= read -r _R_line; do
    [[ -z $_R_line || $_R_line == \#* ]] && continue
    mapfile -td $'\t' _R_f < <(printf %s "$_R_line")
    _R_flags=${_R_f[0]}
    printf -v _R_e %b "${_R_f[1]}"
    _R_items=()
    for _R_s in "${_R_f[@]:2}"; do
        printf -v _R_s %b "$_R_s"
        _R_items+=("$_R_s")
    done

    # Names the case could touch, less any the shell already has
    _R_s="$_R_e ${_R_items[*]}"
    declare -A _R_seen=()
    _R_names=()
    while [[ $_R_s =~ [A-Za-z_][A-Za-z0-9_]* ]]; do
        _R_n=${BASH_REMATCH[0]}
        _R_s=${_R_s#*"$_R_n"}
        [[ $_R_n == _R_* || ${_R_seen[$_R_n]} ]] && continue
        _R_seen[$_R_n]=1
        _R_names+=("$_R_n")
    done
    declare -A _R_pre=()
    for _R_n in "${_R_names[@]}"; do
        declare -p "$_R_n" >/dev/null 2>&1 && _R_pre[$_R_n]=1
    done

    _R_out=$( (_R_setup; echo "= $(($_R_e))") 2>&1 )
    _R_state=$( (_R_setup; (($_R_e)); _R_dump) 2>/dev/null )

    echo "case $_R_flags"
    _R_esc "$_R_e"
    echo "expr $_R_REPLY"
    for _R_s in "${_R_items[@]}"; do
        _R_esc "$_R_s"
        echo "setup $_R_REPLY"
    done
    if [[ $_R_out == "= "* ]]; then
        echo "value ${_R_out#= }"
    elif [[ $_R_out =~ ^[^:]*:\ line\ [0-9]+:\ (.*)$ ]]; then
        _R_esc "${BASH_REMATCH[1]}"
        echo "error $_R_REPLY"
    else
        _R_esc "$_R_out"
        echo "unexpected $_R_REPLY"
    fi
    if [[ $_R_state == DUMP* ]]; then
        _R_state=${_R_state#DUMP}
        _R_state=${_R_state#$'\n'}
        [[ $_R_state ]] && printf '%s\n' "$_R_state"
    else
        echo "nostate"
    fi
    echo "end"
done
