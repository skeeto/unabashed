== stdout 618
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
trap -- 'echo u' SIGUSR1
--
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
trap -- 'echo u' SIGUSR1
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
[trap -- 'echo "[$(trap -p)]"; (trap -p EXIT)' EXIT]
trap -- 'echo "[$(trap -p)]"; (trap -p EXIT)' EXIT
reset
trap -- 'echo second' EXIT
status 3
trap -- 'f' EXIT
status 2
err
trap -- 'echo err; trap -p ERR' ERR
debug
trap -- 'echo debug; trap -p DEBUG' DEBUG
usr1
trap -- 'echo usr1; trap -p USR1' SIGUSR1
term
trap -- 'trap -p EXIT' EXIT
-- subshells in it
sub
bg
[]
[cs]
done

== stderr 0

== merged 618
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
trap -- 'echo u' SIGUSR1
--
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
trap -- 'echo u' SIGUSR1
trap -- 'trap; echo --; trap -p EXIT; trap -p USR1 EXIT' EXIT
[trap -- 'echo "[$(trap -p)]"; (trap -p EXIT)' EXIT]
trap -- 'echo "[$(trap -p)]"; (trap -p EXIT)' EXIT
reset
trap -- 'echo second' EXIT
status 3
trap -- 'f' EXIT
status 2
err
trap -- 'echo err; trap -p ERR' ERR
debug
trap -- 'echo debug; trap -p DEBUG' DEBUG
usr1
trap -- 'echo usr1; trap -p USR1' SIGUSR1
term
trap -- 'trap -p EXIT' EXIT
-- subshells in it
sub
bg
[]
[cs]
done

== status 0
