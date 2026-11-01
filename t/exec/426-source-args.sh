# Positional parameters for sourced files
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/args.sh <<'END'
echo "n=$# 1=${1-none} 2=${2-none} all=$*"
END
set -- outer1 outer2
. /tmp/args.sh
. /tmp/args.sh new1
echo "after: $# $*"
source /tmp/args.sh a b c
echo "after2: $# $*"
f() { . /tmp/args.sh; . /tmp/args.sh z; echo "f: $*"; }
f fa fb
mk > /tmp/shift.sh <<'END'
shift
echo "shifted: $*"
set -- replaced
END
. /tmp/shift.sh
echo "outer now: $*"
. /tmp/shift.sh x y
echo "outer now2: $*"
