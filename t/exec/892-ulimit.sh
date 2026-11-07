# ulimit: printing, setting, its option parsing and errors, and the
# scope of a change (subshells and comsubs keep theirs)
ulimit; ulimit -f; ulimit -c; ulimit -n; ulimit -Hn; ulimit -p
ulimit -t -c; ulimit -H -s; ulimit -S -s
ulimit -c 0 -t; echo "status $?"
ulimit -t 100 200; echo "status $?"; ulimit -t
ulimit -St 50 -c 30 40; echo "status $?"; ulimit -t; ulimit -Ht; ulimit -c
ulimit -f77; ulimit -f
ulimit -Sn 500; ulimit -n; ulimit -Hn
ulimit -n 2000; echo "status $?"
ulimit -Sn 800; echo "status $?"; ulimit -Sn
ulimit -Sn hard; ulimit -Sn
ulimit -Hn 600; echo "status $?"; ulimit -Sn; ulimit -Hn
ulimit -Hn soft; ulimit -Hn
ulimit -p 16; echo "status $?"
ulimit -c 0x10; echo "status $?"
ulimit -c abc; ulimit -c ''; ulimit -c -; ulimit -c ' 5'; ulimit -c +5
ulimit -c 010; ulimit -c; ulimit -c 08; ulimit -c
ulimit -c 99999999999999999999; echo "status $?"
ulimit -c 18014398509481983; echo "status $?"; ulimit -c
ulimit -c 18014398509481984; echo "status $?"
ulimit -tx 5; echo "status $?"
ulimit -t -1; echo "status $?"
ulimit -t 5 -0; echo "status $?"; ulimit -t
ulimit -y; echo "status $?"
ulimit -- 70; ulimit -f
ulimit -c unlimited -t unlimited
( ulimit -t 9; ulimit -t ); ulimit -t
echo "comsub $(ulimit -t 7; ulimit -t)"; ulimit -t
ulimit -t 9 | cat; ulimit -t
f() { ulimit -t 11; }; f; ulimit -t
( ulimit -Ht 20; ulimit -Ht 30; echo "status $?"; ulimit -Ht )
