# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 8):
# descriptors above 2 for CRT programs, and procsubs bridged to native
# programs as named pipes (the vos names them by pid and sequence,
# which varies with scheduling: normalized to @P).
ARGV_FD=3 argv 3>f >/dev/null
ARGV_FD=5 argv | busybox grep fd
busybox cat f
busybox cat <(echo one) <(printf 'two\n')
argv --in=<(true) <(true) <(true) | busybox sed 's/unabashed-[0-9-]*/@P/g'
argv never <(echo unread) >/dev/null
echo "unopened $?"
echo tee | busybox tee >(busybox cat >t1) >/dev/null
wait
busybox cat t1
busybox cat <(exit 3)
echo "empty $?"
busybox wc -l <(busybox seq 1000) | busybox sed 's/unabashed-[0-9-]*/@P/g'
