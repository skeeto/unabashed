# umask: the mask applies to files a redirection creates and to the
# programs the shell starts, in a subshell to that subshell alone, and
# never to a file that exists
modes() { ls -ld "$@" | cut -c1-10; }
cd "$(mktemp -d)"
echo a >f0
umask 077
echo a >f1; echo a >>f2; exec 3<>f3; exec 3>&-
echo a &>f4; echo a >|f5
touch f6; mkdir d1; echo a | tee f7 >/dev/null
echo b >>f0; echo b >f0
modes f0 f1 f2 f3 f4 f5 f6 f7 d1
(umask 002; umask; echo a >g1; touch g2; mkdir d2; modes g1 g2 d2
 (umask 777; echo a >h1; modes h1); echo a >g3; modes g3)
umask; echo a >f8; touch f9 & wait
modes f8 f9
set -C; echo a >f10; modes f10; set +C
f() { umask 027; }; f; echo a >f11; touch f12; modes f11 f12
umask a=rwx; echo "$(umask; echo a >f13; touch f14; modes f13 f14)"
echo a | { umask 077; cat >f15; }; modes f15; umask
umask 022; bash -c 'umask; echo a >f16; modes() { ls -ld "$@" | cut -c1-10; }; modes f16'
umask 077; bash -c 'umask; echo a >f17; touch f18; ls -ld f17 f18 | cut -c1-10'
eval 'umask 026'; echo a >f19; modes f19
