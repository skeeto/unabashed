# exec while other work runs: bash's procsubs and jobs are processes of
# their own, so the program replaces the shell while they run on (its
# pid kept: t/posix/001); with no EXIT trap, and a failed exec as before
bash -c 'exec bash -c "read x; echo \"\$x\"; cat" < <(echo first; sleep 1; echo later)'
bash -c 'exec cat < <(echo a; sleep 1; echo b) 3< <(echo three)'
bash -c 'f() { printf "%s\n" one two three; }; exec sort -r < <(f)'
bash -c 'exec head -n 2 < <(i=0; while :; do echo "l $i"; i=$((i+1)); done)'
bash -c 'exec echo hi > >(tr a-z A-Z)'; sleep 1
bash -c 'trap "echo EXIT trap" EXIT; exec cat < <(echo no-trap)'
bash -c 'exec nosuch < <(echo x)'; echo "nosuch $?"
bash -c 'shopt -s execfail; exec nosuch < <(echo x) 2>/dev/null; echo "still here $?"'
bash -c '{ sleep 1; echo job-done; } & exec cat < <(echo cat-in)'; sleep 2
v=$(bash -c 'exec cat < <(echo captured)'); echo "$v"
