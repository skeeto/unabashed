# set -C refuses to overwrite regular files; >| and >> still work
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
echo first >/tmp/f
set -C
echo second >/tmp/f
echo status $?
show /tmp/f
echo third >|/tmp/f
show /tmp/f
echo fourth >>/tmp/f
show /tmp/f
echo new >/tmp/g
show /tmp/g
echo devnull >/dev/null
echo status $?
echo both &>/tmp/f
echo status $?
set -o noclobber
echo again 2>/tmp/g
echo status $?
set +C
echo fifth >/tmp/f
show /tmp/f
set -C
echo dupword >&/tmp/f
echo status $?
echo rw <>/tmp/f
echo status $?
set +C
