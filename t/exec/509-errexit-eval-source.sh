echo 'echo s1; false; echo s2' > /tmp/s.sh
(set -e; . /tmp/s.sh; echo after-src); echo "st=$?"
(set -e; . /tmp/s.sh || echo rescued; echo after-src2); echo "st=$?"
(set -e; eval 'false; echo e2' || echo rescued; echo after-eval); echo "st=$?"
(set -e; if eval false; then :; else echo else; fi); echo "st=$?"
(set -e; f() { eval "$@"; }; f 'echo in; false; echo out'; echo after); echo "st=$?"
