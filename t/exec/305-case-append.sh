declare -u u=ab
u+=cd
echo "$u"
declare -l l=AB
l+=CD
echo "$l"
declare -u -i ui=3
ui+=4
echo "ui=$ui"
declare -p u l ui
declare -u w="hello world"
echo "$w"
w+=" again"
echo "$w"
