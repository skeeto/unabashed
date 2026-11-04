# Recorder for t/unit/paths.golden: Git Bash's cygpath and msys argument
# and environment conversion on the curated cases in t/unit/paths.cases.
#
# $ GITBASH=C:/Users/wellons/git/usr/bin/bash.exe
# $ WORK=$(mktemp -d)     # an empty scratch directory outside the repo
# $ cat t/unit/paths_record.sh t/unit/paths.cases |
#       $GITBASH --norc --noprofile -s "$WORK" >t/unit/paths.golden
#
# It runs only the curated cases: cygpath -u/-w/-m/-p on fixed strings
# and a throwaway native argv printer (compiled with gcc into WORK) that
# prints the arguments and environment it receives. Nothing touches the
# file system beyond WORK, and no case names a UNC path that cygpath -w
# would look up on the network. WORK must be empty: relative cases
# resolve there.
#
# The header records the context the test rebuilds (@root, @temp, @cwd,
# @drives, @fstab lines from etc/fstab). Each record starts with a kind,
# holds fields, and ends with a "." line. Fields hold bytes with
# printable ASCII as is, backslash as \\ and everything else as \xHH.
#   T  kind: u w m (cygpath -u/-w/-m), up wp mp (with -p), arg (one
#      argument to a native child), excl (with MSYS2_ARG_CONV_EXCL),
#      env (an environment from env -i)
#   X  MSYS2_ARG_CONV_EXCL, for excl
#   I  an input: the path, the argument, or an env entry
#   O  an output line; /proc/N/ shows as /proc/PID/
#
# This is free and unencumbered software released into the public domain.

set -e
export PATH=/usr/bin:$PATH  # cygpath first; gcc from the caller
export LC_ALL=C
unset MSYS2_ARG_CONV_EXCL MSYS2_ENV_CONV_EXCL MSYS_NO_PATHCONV
cd "$(cygpath -u "$1")"
if [ -n "$(ls -A)" ]; then
    echo "paths_record.sh: $1 is not empty" >&2
    exit 1
fi

cat >argv.c <<'EOF'
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
static void put(char *tag, wchar_t *s)
{
    static char buf[1<<16];
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, buf, sizeof(buf), 0, 0);
    fputs(tag, stdout);
    for (int i = 0; i < n-1; i++) {
        unsigned char c = buf[i];
        if (c == '\\') fputs("\\\\", stdout);
        else if (c>=0x20 && c<0x7f) putchar(c);
        else printf("\\x%02x", c);
    }
    putchar('\n');
}
int wmain(int argc, wchar_t **argv)
{
    for (int i = 1; i < argc; i++) put("A ", argv[i]);
    wchar_t *env = GetEnvironmentStringsW();
    for (wchar_t *p = env; *p; p += wcslen(p)+1) put("E ", p);
    return 0;
}
EOF
${CC:-gcc} -municode -o argv.exe argv.c
rm argv.c
mkdir empty
cd empty
ARGV=../argv.exe

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

field() { printf '%s ' "$1"; printf '%s' "$2" | enc; }

# The printer's lines are already encoded
outs()
{
    sed -n "s/^$1 /O /p" | sed 's|/proc/[0-9][0-9]*/|/proc/PID/|g'
}

# For the cases: the install root and %TEMP% in both Windows forms
ROOTM=$(cygpath -m /)
ROOTM=${ROOTM%/}
ROOTW=$(cygpath -w /)
ROOTW=${ROOTW%\\}
TEMPM=$(cygpath -m /tmp)
TEMPW=$(cygpath -w /tmp)

echo "@root $(cygpath -m /)"
echo "@temp $(cygpath -m /tmp)"
echo "@cwd $(cygpath -am .)"
echo "@drives $(sed -n 's/^\([A-Z]\): .*/\1/p' /proc/mounts | tr -d '\n')"
while IFS= read -r line || [ -n "$line" ]; do
    field @fstab "$line"
done </etc/fstab

cyg()
{
    local kind=$1 flags=$2 p=$3 out
    out=$(cygpath $flags -- "$p") || echo "paths_record.sh: cygpath $flags fails on $p" >&2
    printf 'T %s\n' "$kind"
    field I "$p"
    field O "$out"
    printf '.\n'
}

u()  { local p; for p; do cyg u  -u  "$p"; done; }
w()  { local p; for p; do cyg w  -w  "$p"; cyg m -m "$p"; done; }
up() { local p; for p; do cyg up -up "$p"; done; }
wp() { local p; for p; do cyg wp -wp "$p"; cyg mp -mp "$p"; done; }

arg()
{
    local a
    for a; do
        printf 'T arg\n'
        field I "$a"
        $ARGV "$a" | outs A
        printf '.\n'
    done
}

# excl EXCL ARG...: arguments under MSYS2_ARG_CONV_EXCL=EXCL
excl()
{
    local x=$1 a
    shift
    printf 'T excl\n'
    field X "$x"
    for a; do field I "$a"; done
    MSYS2_ARG_CONV_EXCL=$x $ARGV "$@" | outs A
    printf '.\n'
}

# env NAME=VALUE...: what a native child gets from env -i, without the
# variables the runtime fills in from Windows (SYSTEMROOT, WINDIR, MSYSTEM)
env_()
{
    local e
    printf 'T env\n'
    for e; do field I "$e"; done
    env -i "$@" $ARGV | outs E | grep -Eiv '^O (SYSTEMROOT|WINDIR|MSYSTEM)=' || true
    printf '.\n'
}
