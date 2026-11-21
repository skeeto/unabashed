# ulimit -c and -f count blocks of 1024 bytes, or 512 in POSIX mode,
# where they were always 512; the labels stay "blocks". Values are
# unsigned 64-bit, as rlim_t: digits wrap, and only the scaling is
# checked, so a few that once were "out of range" are limits. Each
# case runs in a shell of its own (the limit set in the process) and in
# a subshell, writing to a pipe so that a small file size limit spares
# the output.
cases='ulimit -f 3; ulimit -f; ulimit -Sf; ulimit -Hf; set -o posix; ulimit -f; ulimit -Hf
ulimit -c 5; ulimit -c; ulimit -Hc; set -o posix; ulimit -c
set -o posix; ulimit -f 3; ulimit -f; set +o posix; ulimit -f; ulimit -a | grep -e -f
set -o posix; ulimit -c 1; set +o posix; ulimit -c; ulimit -Hc
ulimit -f 7 -c 9; ulimit -f -c; set -o posix; ulimit -c -f; ulimit -a | grep -e -c -e -f
ulimit -S -f 7; ulimit -H -f 9; ulimit -Hf; ulimit -Sf; ulimit -f hard; ulimit -f
ulimit -Hf 10; ulimit -f 11; echo $?; ulimit -f
ulimit -f 9007199254740991; ulimit -f; ulimit -f 9007199254740993; echo $?; ulimit -f
set -o posix; ulimit -f 18014398509481985; echo $?; ulimit -f
ulimit -f 18014398509481985; echo $?; ulimit -f
ulimit -t 99999999999999999999; echo $?; ulimit -t
ulimit -s 100; ulimit -s; ulimit -d 3000; ulimit -d; ulimit -v 20000; ulimit -v
ulimit -f 08; ulimit -f; ulimit -f 08x; ulimit -f 0x10; ulimit -f 1a; echo $?'
printf '%s\n' "$cases" | while IFS= read -r c; do
    echo "## $c"
    bash -c "$c" bash 2>&1 | cat
    (eval "$c") 2>&1 | cat
done
ulimit -a | grep -e -c -e -f -e -p
set -o posix
ulimit -a | grep -e -c -e -f -e -p
