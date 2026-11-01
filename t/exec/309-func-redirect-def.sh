writer() { echo "line one"; echo "line two"; } > /tmp/out
writer
while read -r l; do echo "read: $l"; done < /tmp/out
appender() { echo "more"; } >> /tmp/out
appender
appender
while read -r l; do echo "read2: $l"; done < /tmp/out
errfunc() { echo "to stderr" >&2; } 2>&1
errfunc
reader() { read -r first; echo "first=$first"; } < /tmp/out
reader
reader
declare -f writer reader
