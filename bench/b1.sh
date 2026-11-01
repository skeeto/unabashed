i=0; while ((i<1000000)); do ((i++)); done
for ((i=0;i<1000000;i++)); do x=$((x+i*3%7)); done
echo $i $x
