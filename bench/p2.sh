# P2 (POSIX): string building and parameter pattern removal
s=; i=0; while [ $i -lt 20000 ]; do s=$s$i; i=$((i+1)); done; echo ${#s}
t=$s; n=0; while [ ${#t} -gt 0 ]; do t=${t%?????}; n=$((n+1)); done; echo $n
u=/usr/local/share/doc/example/file.tar.gz; i=0
while [ $i -lt 100000 ]; do b=${u##*/}; d=${u%/*}; e=${b#*.}; i=$((i+1)); done
echo $b $d $e
