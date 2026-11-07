# P6 (POSIX): here-documents into read, pipelines of builtins, printf
i=0; while [ $i -lt 2000 ]; do read -r x <<EOT
line $i
EOT
i=$((i+1)); done; echo "$x"
i=0; while [ $i -lt 300 ]; do x=$(echo $i | { read -r y; echo $((y*2)); }); i=$((i+1)); done; echo $x
i=0; while [ $i -lt 20000 ]; do printf '%05d %s %x\n' $i str $i; i=$((i+1)); done | { n=0; while read -r a b c; do n=$((n+1)); done; echo $n; }
