#!/bin/sh
# Record t/unit/fakes.golden in the oracle image (tools/Dockerfile.bash53)
#
#   sh t/unit/fakes_record.sh           # rewrite t/unit/fakes.golden
#   sh t/unit/fakes_record.sh compare   # fakes against the real coreutils
#
# FAKES_CASES and FAKES_GOLDEN override the input and output files, and
# FAKES_PATH the PATH for recording (/usr/local/bin:/usr/bin:/bin records
# the real coreutils, for checking fidelity on the vos).
#
# Run where docker works (WSL). Every case in fakes.cases runs under bash
# 5.3 as "bash -c CASE bash" in /home/oracle, freshly populated from
# fakes.fixture, with env -i LANG=C TZ=UTC HOME=/home/oracle and
# PATH=/usr/local/bin:/opt/fakes/bin, stdin /dev/null, user oracle and
# hostname oracle, as in tools/record. The golden blocks follow the spec
# format of tools/README.md, each preceded by "== case LEN" and the case.
#
# "compare" runs each case twice, with the fakes first on the PATH and
# with only the image's real coreutils, and prints the cases that differ.
# Each case gets 30 seconds (status 124 after that), as the real
# coreutils can block, as in "sleep inf".
set -eu

image=${RECORD_IMAGE:-unabashed/bash53}
mode=${1:-record}
dir=$(cd "$(dirname "$0")" && pwd)
cases=$(realpath "${FAKES_CASES:-$dir/fakes.cases}")
out=${FAKES_GOLDEN:-$dir/fakes.golden}
path=${FAKES_PATH:-/usr/local/bin:/opt/fakes/bin}

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*';;
esac

inner='
cases=/cases
mapfile -t fixture </data/fakes.fixture

# Default modes need no chmod under umask 022, which saves processes
setup() {
    cd /
    /usr/bin/chmod -R u+rwx /home/oracle /tmp 2>/dev/null
    /usr/bin/rm -rf /home/oracle/* /home/oracle/.[!.]* /home/oracle/..?* \
                    /tmp/* /tmp/.[!.]* /tmp/..?*
    cd /home/oracle
    local f kind name mode content
    for f in "${fixture[@]}"; do
        read -r kind name mode content <<<"$f"
        case $kind in
        d) /usr/bin/mkdir "$name"
           [ "$mode" = 755 ] || /usr/bin/chmod "$mode" "$name";;
        f) printf "%b" "$content" >"$name"
           [ "$mode" = 644 ] || /usr/bin/chmod "$mode" "$name";;
        l) /usr/bin/ln -s "$mode" "$name";;
        esac
    done
}

run() {  # PATH CASE OUT ERR
    setup
    /usr/bin/timeout -k 5 30 env -i LANG=C TZ=UTC HOME=/home/oracle PATH="$1" \
        /usr/local/bin/bash --norc --noprofile -c "$2" bash \
        </dev/null >"$3" 2>"$4"
    status=$?
    cd /
}

block() {
    printf "== %s %d\n" "$1" "$(/usr/bin/wc -c <"$2")"
    /usr/bin/cat "$2"
    printf "\n"
}

n=0
bad=0
while IFS= read -r line; do
    case $line in ""|"#"*) continue;; esac
    n=$((n + 1))
    if [ "$1" = compare ]; then
        run /opt/fakes/bin:/usr/local/bin "$line" /rec/fo /rec/fe
        fs=$status
        run /usr/local/bin:/usr/bin:/bin "$line" /rec/ro /rec/re
        if ! /usr/bin/cmp -s /rec/fo /rec/ro || ! /usr/bin/cmp -s /rec/fe /rec/re ||
           [ $fs != $status ]; then
            bad=$((bad + 1))
            printf "### %s\n" "$line"
            [ $fs = $status ] || printf "status: fake %d, real %d\n" $fs $status
            /usr/bin/diff --label fake --label real -u /rec/fo /rec/ro || true
            /usr/bin/diff --label fake-err --label real-err -u /rec/fe /rec/re || true
        fi
    else
        run "$2" "$line" /rec/out /rec/err
        printf "== case %d\n%s\n" "${#line}" "$line"
        block stdout /rec/out
        block stderr /rec/err
        printf "== status %d\n" $status
    fi
done <"$cases"
if [ "$1" = compare ]; then
    printf "%d cases, %d differ from coreutils\n" $n $bad
fi
'

run() {
    docker run --rm -i --network none --read-only --hostname oracle \
        --tmpfs /tmp:rw,exec,nosuid,nodev,mode=1777 \
        --tmpfs /home/oracle:rw,exec,nosuid,nodev,uid=1000,gid=1000,mode=755 \
        --tmpfs /rec:rw,nosuid,nodev,uid=1000,gid=1000,mode=755 \
        --workdir /home/oracle --user oracle \
        --cap-drop ALL --security-opt no-new-privileges \
        -v "$dir:/data:ro" -v "$cases:/cases:ro" "$image" \
        /usr/local/bin/bash --norc --noprofile -c "$inner" inner "$1" "$path"
}

if [ "$mode" = compare ]; then
    run compare
else
    tmp=$(mktemp)
    trap 'rm -f "$tmp"' EXIT
    {
        echo "# Recorded by t/unit/fakes_record.sh from fakes.cases; do not edit."
        run record
    } >"$tmp"
    cp "$tmp" "$out"
    echo "$out: $(grep -c "^== case " "$out") cases"
fi
