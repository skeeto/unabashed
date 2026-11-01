set -u
x=set
echo "x=$x"
echo "def=${unset1-dflt}"
echo "def2=${unset1:-dflt2}"
echo "plus=${unset1+alt}."
echo "empty=${empty=}"
echo "len=${#x}"
(echo "a $unset2"; echo after-a); echo "st=$?"
(echo "b ${unset3}"; echo after-b); echo "st=$?"
(echo "c ${#unset4}"; echo after-c); echo "st=$?"
(echo "d ${unset5#x}"; echo after-d); echo "st=$?"
(echo "e ${unset6/a/b}"; echo after-e); echo "st=$?"
(echo "f ${unset7:1}"; echo after-f); echo "st=$?"
(: $unset8; echo after-g); echo "st=$?"
echo end
echo "$unset9"
echo notreached
