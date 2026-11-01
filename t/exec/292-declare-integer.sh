declare -i n=5*3
echo "n=$n"
n=n+1
echo "n=$n"
declare -i o=010 h=0x1f b=2#101 base36=36#z
echo "$o $h $b $base36"
declare -i e=
echo "empty=[$e]"
declare -i s
s=" 7 "
echo "s=[$s]"
q=4
declare -i r=q*q
echo "r=$r"
declare -i bad
bad="1 +"
echo "status $? bad=[$bad]"
bad=3
bad='2#9'
echo "status $? bad=[$bad]"
declare -p n o h b e s r bad
