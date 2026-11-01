declare -i n=5
n+=3
echo "n=$n"
n+="2*3"
echo "n=$n"
n+=-20
echo "n=$n"
s=5
s+=3
echo "s=$s"
declare -i z
z+=7
echo "z=$z"
declare -i w=10
w+=w
echo "w=$w"
declare -i neg=-5
neg+=1
echo "neg=$neg"
declare -i big=9223372036854775807
big+=1
echo "big=$big"
declare -i n+=100
echo "declare append n=$n"
local_test() { local -i li=1; li+=li+1; echo "li=$li"; }
local_test
