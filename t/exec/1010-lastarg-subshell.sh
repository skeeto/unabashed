# A subshell started while the script's own value of _ is still bound
# rebinds _ as its own: the level around it keeps its value
for _ in a b; do ( : sub ); echo "got $_"; done
f() { local _=L; ( : x ); echo "f $_"; }
f
for _ in c; do x=$(: y; echo "in $_"); echo "$x, out $_"; done
for _ in d; do (export _; ( : z )); echo "$_"; done
