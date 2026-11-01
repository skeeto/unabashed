(set -e; echo a; false; echo b); echo "st=$?"
(set -e; echo a; (exit 3); echo b); echo "st=$?"
(set -e; true; echo c); echo "st=$?"
(set -e; nosuchcmd; echo d); echo "st=$?"
(set -e; f() { return 4; }; f; echo e); echo "st=$?"
(set -o errexit; false; echo f); echo "st=$?"
(set -e; set +e; false; echo g); echo "st=$?"
set -e
echo top
false
echo notreached
