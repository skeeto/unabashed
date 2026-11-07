# Waiting for more programs at once than WaitForMultipleObjects allows
for i in $(seq 70); do "$U" -c 'exit 0' & done
wait
# If $!'s program ended before wait began, as bash, wait keeps its
# status for wait -n below: jobs reports and drops it
jobs >/dev/null
echo "all done"
for i in $(seq 66); do "$U" -c "exit $((i % 5))" & done
n=0 sum=0
while wait -n; st=$?; [ $st -ne 127 ]; do n=$((n+1)); sum=$((sum+st)); done
echo "n=$n sum=$sum"
"$U" -c 'exit 7' & p=$!
for i in $(seq 65); do "$U" -c 'exit 0' & done
wait $p; echo "st=$?"
wait
