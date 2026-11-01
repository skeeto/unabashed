(echo "a=$((1/0))"; echo after-a); echo "st=$?"
(echo "b=$((5%0))"; echo after-b); echo "st=$?"
(x=0; echo "c=$((10/x))"; echo after-c); echo "st=$?"
(x=$((2/0)); echo after-d); echo "st=$?"
(echo "e=$((1+))"; echo after-e); echo "st=$?"
(echo "f=$((08))"; echo after-f); echo "st=$?"
(x=5; : $((x/=0)); echo "after-g x=$x"); echo "st=$?"
echo "top=$((3/0))"
echo notreached
