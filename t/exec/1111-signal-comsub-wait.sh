# An untrapped deadly signal sent to the shell while it waits, for a
# command substitution or in the wait builtin, takes effect there, as
# bash's handler acts in the wait: the rest of the command neither
# expands nor runs, and the EXIT action sees $? from before the wait.
# A trapped one runs its action once the command is done.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c" 2>&1 | cat
    echo "status ${PIPESTATUS[0]}"
done <<'EOF'
[[ $(kill $$) ]]; echo after
trap 'echo "exit $?"' EXIT; [[ $(kill $$) ]]; echo after
trap 'echo "exit $?"' EXIT; x=$(kill $$)$(echo second >&2); echo after
trap 'echo "exit $?"' EXIT; case $(kill $$) in *) echo case;; esac
trap 'echo "exit $?"' EXIT; for i in $(kill $$) 1; do echo body; done
trap 'echo "exit $?"' EXIT; (( $(kill $$; echo 1) )); echo after
trap 'echo "exit $?"' EXIT; f() { x=$(kill $$); echo "in f"; }; f
trap 'echo "exit $?"' EXIT; x=$(kill -HUP $$); echo after
trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait; echo after
trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait $!; echo after
trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait -n; echo after
trap 'echo "exit $?"' EXIT; (sleep 0.1; kill $$) & wait; echo after
trap 'echo "usr1 $?"' USR1; echo "$(kill -USR1 $$; echo in) after"; echo next
trap 'echo "usr1 $?"' USR1; f() { kill -USR1 $$; }; f & wait; echo "after $?"
EOF
