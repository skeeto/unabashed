declare -i n=5
declare +i n
n=2+3
echo "n=$n"
declare -x e=1
declare +x e
declare -p e
declare -u u=a
declare +u u
u=lower
echo "u=$u"
declare -il il=7
declare +i il
declare -p il
declare -l q=A
declare +l q=B
echo "q=$q"
declare -p n u q
