# "! cmd &" is not inverted: the job's status, as wait reports it, is
# cmd's own, whatever cmd is, so an ERR trap or set -e sees that. In
# the job ! still turns set -e and ERR off, and a compound command
# whose own redirection fails exits 0 (bash's execute_in_subshell). The
# job's text drops the ! for a simple command or a pipeline only.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "trap 'echo err' ERR; $c; echo \"status \$?\""
done <<'EOF'
! true & wait $!
! false & wait $!
! { echo a; } & wait $!
! { false; } & wait $!
! { false; echo a; } & wait $!
! (true) & wait $!
! (exit 3) & wait $!
! true | true & wait $!
! if true; then true; fi & wait $!
! for i in 1; do false; done & wait $!
! [[ x ]] & wait $!
! ((1)) & wait $!
! true >/nonexist/x & wait $!
! { true; } >/nonexist/x & wait $!
! true & ! true; echo "fg $?"; wait $!
! ! true & wait $!
set -e; ! { false; echo in; } & wait $!
set -m; ! true & wait $!
f() { ! true & wait $!; }; f
! sleep 1 & jobs; wait
! sleep 1 | true & jobs; wait
! { sleep 1; } & jobs; wait
EOF
