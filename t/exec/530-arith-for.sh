for (( i = 0; i < 3; i++ )); do echo "i=$i"; done
echo "after i=$i"
for ((j=10; j>0; j-=4)); do echo "j=$j"; done
for (( a=0, b=5; a<b; a++, b-- )); do echo "a=$a b=$b"; done
k=0
for (( ; k < 2 ; )); do echo "k=$k"; (( k++ )); done
for (( ;; )); do (( k++ )); if (( k > 5 )); then break; fi; done; echo "k=$k"
for (( n=0; n<5; n++ )); do
  (( n == 1 )) && continue
  (( n == 3 )) && break
  echo "n=$n"
done
for (( x=0; x<2; x++ )); do for (( y=0; y<2; y++ )); do echo "$x$y"; done; done
for (( p=0; p<2; p++ )) { echo "brace p=$p"; }
for (( q=0; q<0; q++ )); do echo never; done; echo "empty loop status $?"
false
for (( r=0; r<1; r++ )); do true; done; echo "status $?"
for (( s=0; s<3; s++ )); do (( s == 2 )); done; echo "last body status $?"
lim=2
for (( t=0; t<lim; t++ )); do echo "t=$t"; done
for (( u=0; u<$lim; u++ )); do echo "u=$u"; done
for ((v=3;v;v--)); do printf '%s ' $v; done; echo
