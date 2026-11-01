x=$(echo a; echo "${bad x}"; echo b); echo "1 st=$? x=$x"
x=$(echo a; : $((1/0)); echo b); echo "2 st=$? x=$x"
x=$(set -u; echo a; echo $nope; echo b); echo "3 st=$? x=$x"
x=$(echo a; : ${nope?gone}; echo b); echo "4 st=$? x=$x"
x=$(exit abc; echo b); echo "5 st=$? x=$x"
x=$(readonly r=1; r=2; echo b); echo "6 st=$? x=$x"
x=$(nosuch); echo "7 st=$?"
echo "8 [$(echo ${bad x})] st=$?"
echo end
