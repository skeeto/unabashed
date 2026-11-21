== stdout 1223
-- [[ $(kill $$) ]]; echo after
status 143
-- trap 'echo "exit $?"' EXIT; [[ $(kill $$) ]]; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; x=$(kill $$)$(echo second >&2); echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; case $(kill $$) in *) echo case;; esac
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; for i in $(kill $$) 1; do echo body; done
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; (( $(kill $$; echo 1) )); echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { x=$(kill $$); echo "in f"; }; f
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; x=$(kill -HUP $$); echo after
exit 0
status 129
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait $!; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait -n; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; (sleep 0.1; kill $$) & wait; echo after
exit 0
status 143
-- trap 'echo "usr1 $?"' USR1; echo "$(kill -USR1 $$; echo in) after"; echo next
in after
usr1 0
next
status 0
-- trap 'echo "usr1 $?"' USR1; f() { kill -USR1 $$; }; f & wait; echo "after $?"
usr1 138
after 138
status 0

== stderr 0

== merged 1223
-- [[ $(kill $$) ]]; echo after
status 143
-- trap 'echo "exit $?"' EXIT; [[ $(kill $$) ]]; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; x=$(kill $$)$(echo second >&2); echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; case $(kill $$) in *) echo case;; esac
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; for i in $(kill $$) 1; do echo body; done
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; (( $(kill $$; echo 1) )); echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { x=$(kill $$); echo "in f"; }; f
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; x=$(kill -HUP $$); echo after
exit 0
status 129
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait $!; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; f() { kill $$; }; f & wait -n; echo after
exit 0
status 143
-- trap 'echo "exit $?"' EXIT; (sleep 0.1; kill $$) & wait; echo after
exit 0
status 143
-- trap 'echo "usr1 $?"' USR1; echo "$(kill -USR1 $$; echo in) after"; echo next
in after
usr1 0
next
status 0
-- trap 'echo "usr1 $?"' USR1; f() { kill -USR1 $$; }; f & wait; echo "after $?"
usr1 138
after 138
status 0

== status 0
