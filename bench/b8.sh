# B8: pipelines of builtins, without and with lastpipe, and a long
# pipeline into a while-read loop
for i in {1..5000}; do echo $i | read x; done; echo "${x-unset}"
shopt -s lastpipe
for i in {1..5000}; do echo $i | read x; done; echo "$x"
shopt -u lastpipe
printf '%s\n' {1..100000} | { n=0; while read x; do n=$((n+x)); done; echo $n; }
