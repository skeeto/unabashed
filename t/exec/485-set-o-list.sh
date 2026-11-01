set -o
echo ---
set +o
echo ---
set -e -o pipefail -u
set -o | { while read -r name val; do case $name in errexit|pipefail|nounset|xtrace|braceexpand|hashall|posix) echo "$name=$val";; esac; done; }
set +o | { while read -r a b c; do case $c in errexit|pipefail|nounset|posix) echo "$a $b $c";; esac; done; }
