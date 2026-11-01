shopt
echo ---
shopt -p
echo ---
shopt -s
echo ---
shopt -u | { n=0; while read -r l; do n=$((n+1)); done; echo "unset count nonzero: $((n > 0))"; }
shopt -o | { while read -r name val; do case $name in errexit|pipefail) echo "$name $val";; esac; done; }
shopt -po errexit noclobber
