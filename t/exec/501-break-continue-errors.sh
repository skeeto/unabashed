break
echo "a st=$?"
continue
echo "b st=$?"
for i in 1 2 3; do break 0; echo "body $i"; done
echo "c st=$?"
for i in 1 2; do continue 0; echo "cbody $i"; done
echo "d st=$?"
for i in 1 2; do break -1; echo "neg $i"; done
echo "f st=$?"
for i in 1 2; do for j in a b; do break 5; done; echo "outer $i"; done
echo "g st=$? i=$i"
for i in 1 2; do for j in a b; do continue 2; echo no; done; echo no; done
echo "h st=$?"
f() { break; }
for i in 1 2; do f; echo "f-body $i"; done
echo "i st=$?"
g() { continue; }
for i in 1 2; do g; echo "g-body $i"; done
while :; do (break); echo "sub-body"; break; done
echo "j st=$?"
echo end
