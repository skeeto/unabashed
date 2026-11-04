# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 5).
# busybox-w32's path rules: /x is the root of the current drive, C:foo
# is relative to that drive's own working directory (=C: inherited from
# the environment, then kept by cd), and PATH and CDPATH split on ;.
# pwd -W is accepted; pwd's usage text is unverified.
cd D: && mkdir dd && cd dd && pwd
cd C: && pwd
mkdir cc && cd D: && pwd
cd C:cc && pwd
cd D: && pwd
cd C: && pwd
cd C:.. && pwd
cd /Windows && pwd
cd / && pwd
cd /work && pwd -W
echo C:cc
[ -d C:cc ] && echo C:cc is a directory
[ -d D:cc ] || echo D:cc is not
cd D:/ && [ -d dd ] && echo dd in D:/
CDPATH='C:/nosuch;C:/work' && cd cc
echo "status $?"
PATH='C:/nosuch;C:/w64/bin' && type -p sh
pwd -P
pwd -X
echo "status $?"
