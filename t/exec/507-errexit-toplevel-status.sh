set -e
f() { echo f; (exit 42); echo no; }
f
echo notreached
