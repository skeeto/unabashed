# personality: msys
# Hand-checked baseline (M9 step 5): an MSYS2 program is given a
# temporary file for a /dev/fd/N argument, as it cannot use the named
# pipe bridges: filled from a procsub it reads before it starts, passed
# on to one it writes after it exits, deleted after it (the vos names
# them by pid and sequence: normalized to @F).
norm() { sed 's/sh-fd-[0-9-]*/@F/g'; }
cat <(echo one) <(printf 'two\n')
wc -l <(seq 1000) | norm
cat <(exit 3)
echo "empty $?"
echo tee | tee >(tr a-z A-Z) >/dev/null
wait
echo hi | tee >(cat >t1) >/dev/null
wait
cat t1
cat --file=<(echo x) 2>&1 | norm
exec 3<t1
cat /dev/fd/3 | cat
ls -A C:/Users/oracle/AppData/Local/Temp; echo "left $?"
