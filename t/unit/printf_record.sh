# Recorder for t/unit/printf.golden: run bash 5.3 on the cases.
#
# $ cat t/unit/printf_record.sh t/unit/printf.cases |
#   docker run --rm -i --network none -e TZ=UTC unabashed/bash53 \
#       /usr/local/bin/bash --norc --noprofile -s >t/unit/printf.golden
#
# Each record starts with a kind and a locale (C or U for C.UTF-8) and
# ends with a "." line. Fields hold bytes with printable ASCII as is,
# backslash as \\ and everything else as \xHH. Kinds:
#   T  printf: F format, A arguments, O stdout, E stderr (one message
#      per line, without the "bash: line N: " prefix), S status, and
#      V name=value after a %n case
#   X  echo -e: A the one argument, O the output of echo -e "$A" X
#
# This is free and unencumbered software released into the public domain.

enc()
{
    od -An -v -tx1 | awk '
        BEGIN { for (i = 0; i < 256; i++) h[sprintf("%02x", i)] = i }
        {
            for (k = 1; k <= NF; k++) {
                v = h[$k]
                if (v == 92) printf "\\\\"
                else if (v >= 32 && v < 127) printf "%c", v
                else printf "\\x%s", $k
            }
        }
        END { printf "\n" }'
}

# Cap file writes (512-byte blocks) so a runaway width fails fast
ulimit -f 2048

loc=C
C() { export LC_ALL=C; loc=C; }
U() { export LC_ALL=C.UTF-8; loc=U; }

record()
{
    local var=$1 f=$2
    shift 2
    printf 'T %s\n' $loc
    printf 'F '; printf '%s' "$f" | enc
    local a
    for a in "$@"; do
        printf 'A '; printf '%s' "$a" | enc
    done
    [[ $var ]] && unset "$var"
    printf "$f" "$@" >/tmp/out 2>/tmp/err
    local s=$?
    printf 'O '; enc </tmp/out
    printf 'E '; sed 's/^[^:]*: line [0-9]*: //' /tmp/err | enc
    printf 'S %d\n' $s
    if [[ $var ]]; then
        printf 'V '; printf '%s=%s' "$var" "${!var}" | enc
    fi
    printf '.\n'
}

# t FORMAT ARG...: a printf case
t() { record '' "$@"; }

# tv VAR FORMAT ARG...: a printf case that assigns VAR with %n
tv() { record "$@"; }

# x STRING: an echo -e case
x()
{
    printf 'X %s\n' $loc
    printf 'A '; printf '%s' "$1" | enc
    printf 'O '; echo -e "$1" X | enc
    printf '.\n'
}

C
