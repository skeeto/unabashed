for i in {1..200000}; do s+=x; done; echo ${#s}
t=; for i in {1..50000}; do t="$t$i"; done; echo ${#t}
