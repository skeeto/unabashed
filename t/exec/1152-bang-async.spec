== stdout 1132
-- ! true & wait $!
status 0
-- ! false & wait $!
err
status 1
-- ! { echo a; } & wait $!
a
status 0
-- ! { false; } & wait $!
err
status 1
-- ! { false; echo a; } & wait $!
a
status 0
-- ! (true) & wait $!
status 0
-- ! (exit 3) & wait $!
err
status 3
-- ! true | true & wait $!
status 0
-- ! if true; then true; fi & wait $!
status 0
-- ! for i in 1; do false; done & wait $!
err
status 1
-- ! [[ x ]] & wait $!
status 0
-- ! ((1)) & wait $!
status 0
-- ! true >/nonexist/x & wait $!
bash: line 1: /nonexist/x: No such file or directory
err
status 1
-- ! { true; } >/nonexist/x & wait $!
bash: line 1: /nonexist/x: No such file or directory
status 0
-- ! true & ! true; echo "fg $?"; wait $!
fg 1
status 0
-- ! ! true & wait $!
status 0
-- set -e; ! { false; echo in; } & wait $!
in
status 0
-- set -m; ! true & wait $!
status 0
-- f() { ! true & wait $!; }; f
status 0
-- ! sleep 1 & jobs; wait
[1]+  Running                    sleep 1 &
status 0
-- ! sleep 1 | true & jobs; wait
[1]+  Running                    sleep 1 | true &
status 0
-- ! { sleep 1; } & jobs; wait
[1]+  Running                    ! { sleep 1; } &
status 0

== stderr 0

== merged 1132
-- ! true & wait $!
status 0
-- ! false & wait $!
err
status 1
-- ! { echo a; } & wait $!
a
status 0
-- ! { false; } & wait $!
err
status 1
-- ! { false; echo a; } & wait $!
a
status 0
-- ! (true) & wait $!
status 0
-- ! (exit 3) & wait $!
err
status 3
-- ! true | true & wait $!
status 0
-- ! if true; then true; fi & wait $!
status 0
-- ! for i in 1; do false; done & wait $!
err
status 1
-- ! [[ x ]] & wait $!
status 0
-- ! ((1)) & wait $!
status 0
-- ! true >/nonexist/x & wait $!
bash: line 1: /nonexist/x: No such file or directory
err
status 1
-- ! { true; } >/nonexist/x & wait $!
bash: line 1: /nonexist/x: No such file or directory
status 0
-- ! true & ! true; echo "fg $?"; wait $!
fg 1
status 0
-- ! ! true & wait $!
status 0
-- set -e; ! { false; echo in; } & wait $!
in
status 0
-- set -m; ! true & wait $!
status 0
-- f() { ! true & wait $!; }; f
status 0
-- ! sleep 1 & jobs; wait
[1]+  Running                    sleep 1 &
status 0
-- ! sleep 1 | true & jobs; wait
[1]+  Running                    sleep 1 | true &
status 0
-- ! { sleep 1; } & jobs; wait
[1]+  Running                    ! { sleep 1; } &
status 0

== status 0
