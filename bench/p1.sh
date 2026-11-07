# P1 (POSIX): arithmetic in while loops
i=0; while [ $i -lt 300000 ]; do i=$((i+1)); done; echo $i
i=0; x=0; while [ $i -lt 200000 ]; do x=$(( (x + i*3) % 7919 )); i=$((i+1)); done; echo $x
