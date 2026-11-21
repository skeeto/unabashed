# An element's subscript may assign or replace the array: the element
# is looked up afterwards, also in a subshell, where the assignment
# copies the array
a=(x y z); echo "${a[a[1]=7,1]}"
a=(x y z); ( echo "sub ${a[a[1]=7,1]}" )
a=(x y z); ( echo "sub ${a[a[5]=7,5]} ${#a[@]}" )
a=(x y z); echo "${a[${ a=(p q r); }1]}"
a=(x y z); ( echo "sub ${a[${ a=(p q r); }1]}" )
declare -A h=([k]=v); ( echo "sub ${h[${ h[k]=w; }k]}" )
declare -A h=([k]=v); echo "${h[${ h=([k]=new); }k]}"
f() { local -a l=(x y z); echo "f ${l[l[2]=q,2]}"; }; f
declare -n r='a[a[0]=first,0]'; a=(x y); echo "$r"
