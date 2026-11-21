# A program's redirection abandoned by an error inside ${ cmd; } leaves
# no trace in the shell: set -e still applies afterwards, xtrace's depth
# is unchanged, and repeating it does not run out of anything
declare -i n
set -x
cat </dev/null > ${ eval 'n=1+'; }
echo traced
cat </dev/null > ${ . /dev/stdin <<<'n=1+'; }
echo traced2
set +x
i=0
while [ $i -lt 200 ]; do cat </dev/null > ${ n=1+; }; i=$((i+1)); done 2>/dev/null
echo done $i
cat </dev/null > ${ if : ${nope?}; then :; fi; }
echo "status $?"
set -e
false
echo "survived false under set -e"
