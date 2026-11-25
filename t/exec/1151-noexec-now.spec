== stdout 719
-- set -n; echo $?
status 0
-- set -o noexec; echo a
status 0
-- { set -n; echo in; }
status 0
-- f() { set -n; echo in; }; f
status 0
-- eval 'set -n; echo in'
status 0
-- echo 'set -n; echo in' >src.sh; . ./src.sh
status 0
-- if set -n; then echo then; fi
status 0
-- set -n && echo and
status 0
-- set -n; set +n
status 0
-- set -n; exit 3
status 0
-- false; set -n
status 0
-- for i in 1 2; do echo $i; set -n; done
1
status 0
-- trap 'set -n' USR1; kill -USR1 $$
status 0
-- x=abc; set -n; echo ${x@x}
status 0
-- ( set -n; echo in ); echo "sub $?"
sub 0
after
exit 0
status 0
-- x=$(set -n; echo in); echo "comsub <$x>"
comsub <>
after
exit 0
status 0
-- set -n | cat; echo "pipe $?"
pipe 0
after
exit 0
status 0

== stderr 0

== merged 719
-- set -n; echo $?
status 0
-- set -o noexec; echo a
status 0
-- { set -n; echo in; }
status 0
-- f() { set -n; echo in; }; f
status 0
-- eval 'set -n; echo in'
status 0
-- echo 'set -n; echo in' >src.sh; . ./src.sh
status 0
-- if set -n; then echo then; fi
status 0
-- set -n && echo and
status 0
-- set -n; set +n
status 0
-- set -n; exit 3
status 0
-- false; set -n
status 0
-- for i in 1 2; do echo $i; set -n; done
1
status 0
-- trap 'set -n' USR1; kill -USR1 $$
status 0
-- x=abc; set -n; echo ${x@x}
status 0
-- ( set -n; echo in ); echo "sub $?"
sub 0
after
exit 0
status 0
-- x=$(set -n; echo in); echo "comsub <$x>"
comsub <>
after
exit 0
status 0
-- set -n | cat; echo "pipe $?"
pipe 0
after
exit 0
status 0

== status 0
