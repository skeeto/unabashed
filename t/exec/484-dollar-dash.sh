echo "init=$-"
set -e
echo "e=$-"
set -u
echo "eu=$-"
set +e -x
echo "ux=$-" 2>/dev/null
set +x
set -f -a -b -C -h -k
echo "many=$-"
set +fabCkh -u
echo "back=$-"
set -o pipefail -o noclobber
echo "pipefail=$-"
set -H 2>/dev/null; echo "H=$-"
set -T -E -P
echo "TEP=$-"
set +TEP +C +H
echo "final=$-"
(set -e; echo "sub=$-")
echo "cs=$(echo $-)"
set -v 2>/dev/null
echo "v=$-" 2>/dev/null
set +v 2>/dev/null
set -o posix
echo "posix=$-"
