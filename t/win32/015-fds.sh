# Descriptors above 2 on real processes (M5a step 8): passed in the
# CRT's lpReserved2 blob to MSVCRT programs and to our own shell, of
# both widths from both shell widths; inherited from busybox (a real
# CRT parent); procsubs given to native programs as named pipes, read
# and written, from the msys persona (WORKDIR/bin/unabashed.exe) and the
# native one (a copy as sh.exe). A 32-bit shell writes 4-byte slots
# even for a 64-bit child: WOW64 widens them. Pipe names are normalized
# to @P, the test directory to @D.
D=$(pwd -W)
norm() {
    local l
    while IFS= read -r l; do
        l=${l//"${D//\//\\}"/@D}
        l=${l//"$D"/@D}
        printf '%s\n' "$l"
    done | "$BB" sed 's/unabashed-[0-9]*-[0-9a-f]*/@P/g'
}
cp "$BIN/argv.exe" "$BIN/argv32.exe" .
cp "$U" sh.exe
U32=$BIN/unabashed32.exe

echo "== to our own shell"
"$U" -c 'echo "u64 $#" >&3' 3>f1
"$U32" -c 'echo "u32 $#" >&3' 3>>f1
"$U32" -c '"$1" -c "echo u32-u64 >&5"' sh "$U" 5>>f1
"$U" -c '"$1" -c "echo u64-u32 >&5"' sh "$U32" 5>>f1
"$U" -c 'echo pipe >&3' 3>&1 | cat
"$U" -c '[ -e /dev/fd/7 ] && echo has7; [ -e /dev/fd/8 ] || echo no8' 7</dev/null
"$U" -c 'exec 3>&-; echo closed >&3' 3>&1 2>&1 | norm
"$U" -c 'read -r x <&4; echo "read $x"' 4<f1
cat f1

echo "== to CRT programs"
ARGV_FD=3 ./argv 3>f2 | "$BB" grep '^fd'
ARGV_FD=3 ./argv32 3>>f2 | "$BB" grep '^fd'
"$U32" -c 'ARGV_FD=4 ./argv 4>>f2; ARGV_FD=4 ./argv32 4>>f2' | "$BB" grep '^fd'
ARGV_FD=63 ./argv 63>>f2 | "$BB" grep '^fd'
ARGV_FD=1 ./argv 5>/dev/null | "$BB" grep '^fd'
ARGV_FD=5 ./argv | "$BB" grep '^fd'
"$BB" od -c f2 | "$BB" sed 's/  */ /g'
"$BB" sh -c 'echo "bb $0" >&3' fd3 3>f3
cat f3

echo "== from a CRT parent"
"$BB" sh -c 'exec 4>f4; "$1" -c "echo via-bb >&4"; "$2" -c "echo via-bb32 >&4"' sh "$U" "$U32"
cat f4

echo "== procsubs to native programs"
"$BB" cat <(echo one) <(printf 'two\n')
"$BB" paste <(printf 'a\nb\n') <(printf 'c\nd\n')
# busybox diff stats each file first, through a handle that takes the
# pipe's one instance (notes/issues.md)
"$BB" diff <(printf 'a\n') <(printf 'b\n') 2>&1 | "$BB" sed "s/ '.*//"
echo "diff status ${PIPESTATUS[0]}"
"$BB" cat <(for i in 1 2 3; do echo "loop $i"; done)
"$BB" wc -l <("$BB" seq 100000) | norm
"$BB" cat - <(echo second) <<<first
./argv --in=<(true) <(true) <(true) | norm
./argv never <(echo unread) >/dev/null
echo "unopened status $?"
echo tee | "$BB" tee >(cat >t1) >(read -r x; echo "shell got $x" >t2) >/dev/null
wait
cat t1 t2
./sh.exe -c '"$BB" cat <(echo native) && "$BB" tee >(cat) <<<native-tee >/dev/null; wait'
"$U32" -c '"$BB" cat <(echo w32)'
./sh.exe -c 'read -r x <"$1"; echo "self $x"' sh <(echo direct)
"$BB" cat <(exit 3)
echo "empty status $?"
