# A shell stage writing into a program's full pipe must not block the
# whole shell: the last stage waits for a program before it reads
s=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
s=$s$s$s$s$s$s$s$s$s$s
s=$s$s$s$s$s$s$s$s$s$s
f() { i=0; while [ $i -lt 30 ]; do echo "$s"; i=$((i+1)); done; }
f | "$U" -c cat | { "$U" -c 'sleep 1'; n=0; while read l; do n=$((n+1)); done; echo $n; }
