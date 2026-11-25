# An expansion error in the header of a case, for, select, (( )), [[ ]]
# or arithmetic for that no simple command or case, for or select body
# holds leaves bash's line at that command's, and the parser counts on
# from there
case $((1.5)) in
esac
echo "1: $LINENO"
{ for i in $((1.5)); do :
  done; }
echo "2: $LINENO"
for i in 1; do case $((1.5)) in
esac; done
echo "3: $LINENO"
f() { case $((1.5)) in
esac
}
f
echo "4: $LINENO"
while for ((i=$((1.5)); i<1; i++)); do :; done
do :; done
echo "5: $LINENO"
eval 'case $((1.5)) in
esac
echo "6: $LINENO"'
echo "7: $LINENO"
case x in
x) for i in $((1.5)); do :
   done ;;
esac
echo "8: $LINENO"
if select i in $((1.5)); do :
done; then :; fi
echo "9: $LINENO"
