# Spawning real programs by kind (M5a step 7), from the msys persona
# (WORKDIR/bin/unabashed.exe) and the native one (a copy as sh.exe):
# MSVCRT programs of both widths from both shell widths, batch files
# through the real cmd.exe, shebangs to busybox, exec with kill-on-close,
# and a program missing a DLL. (MSYS2 programs cannot start under this
# sandbox's token: t/msys covers them.) Paths are normalized: @D the
# test directory, @B WORKDIR/bin.
D=$(pwd -W)
B=$(cd "$BIN" && pwd -W)
norm() {
    local l
    while IFS= read -r l; do
        l=${l//"${D//\//\\}"/@D}
        l=${l//"$D"/@D}
        l=${l//"${B//\//\\}"/@B}
        l=${l//"$B"/@B}
        printf '%s\n' "$l"
    done
}
cp "$BIN/argv.exe" "$BIN/argv32.exe" "$BIN/nodll.exe" .
cp "$U" sh.exe

echo "== widths"
for s in "$U" "$BIN/unabashed32.exe"; do
    "$s" -c './argv "a b" "q\"q" "" "x\\" /c/x "*"; ./argv32 "a\\\"b"' 2>&1 | norm
done
./sh.exe -c './argv "a b" /c/x "*"' 2>&1 | norm

echo "== batch"
printf '@echo off\r\necho [%%1] [%%2] [%%3] [%%4] [%%5]\r\nexit /b 3\r\n' >t.bat
./t.bat 'a&b' '%PATH%' '"q"' 'x^y' 'a|b'
echo "status $?"
cp t.bat t.cmd
./sh.exe -c "./t.cmd '<x>' '%%' 'c d'"
echo "status $?"

echo "== shebangs"
printf '#!%s sh\necho "bb $# $1"\n' "$BB" >s1
printf '#!/usr/bin/env argv\n' >s2
printf '#!/bin/sh\necho "self $1"\n' >s3
printf 'echo "dotsh $1"\n' >s4.sh
chmod +x s1 s2 s3
./s1 one
./sh.exe -c './s1 two; PATH="$PWD;$PATH" ./s2 three; ./s3 four; ./s4.sh five' 2>&1 | norm
./s3 six

echo "== exec"
# The shell execs a program and is killed (by its Windows pid, as $! is
# this shell's own): the program must die with it
"$U" -c 'echo $$ >opid; exec "$BB" sh -c "sleep 3; echo alive >late"' &
"$BB" sleep 1
kill -9 "$(<opid)"
wait
"$BB" sleep 4
[ -e late ] && echo "survived" || echo "died with the shell"
"$U" -c 'exec ./argv32 exec' | norm
"$U" -c 'exec "$BB" sh -c "exit 5"'
echo "status $?"

echo "== missing DLL"
./nodll x 2>&1 | norm
echo "status ${PIPESTATUS[0]}"
./sh.exe -c './nodll y' 2>&1 | norm
echo "status ${PIPESTATUS[0]}"
