declare -i i=1
declare -ir ir=2
declare -ix ix=3
declare -lx lx=LOWER
declare -ur ur=upper
declare -xr xr=4
declare -t tr=5
declare -iulx all=6
declare -p i ir ix lx ur xr tr all
declare -p missing
echo "status $?"
declare -p i missing ir
echo "status $?"
declare novalue
declare -p novalue
declare -i inovalue
declare -p inovalue
