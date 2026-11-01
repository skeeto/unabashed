# kill: -l and -L, signal specs, errors, signal 0
kill; echo $?
kill -l 999; echo $?
kill -l foo; echo $?
kill -l 129 2 SIGHUP hup; echo $?
kill -s; echo $?
kill -n x 1; echo $?
kill -s FOO 1; echo $?
kill x; echo $?
kill %1; echo $?
kill -9; echo $?
kill -0 $$; echo $?
kill -s 0 $$; echo $?
kill -n 0 $$; echo $?
kill -nTERM 99999; echo $?
kill -- -1x; echo $?
kill -L 3; echo $?
kill -l -- 3; echo $?
kill -l 0 64 65; echo $?
kill -l 32 33; echo $?
kill -l RTMIN+3 sigrtmax-1; echo $?
kill -1x 99999; echo $?
kill -x 1; echo $?
kill -l 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 | tr '\n' ' '; echo
kill -L
