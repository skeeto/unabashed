# break and continue inside subshells within loops
for i in 1 2 3; do (break); echo "a$i"; done
for i in 1 2 3; do x=$(continue); echo "b$i"; done
for i in 1 2; do (for j in x y; do break; done; echo "in $j"); done
(break 2>&1)
echo "st=$?"
for i in 1 2; do ( if [ $i = 1 ]; then exit 1; fi; echo "c$i" ); done
