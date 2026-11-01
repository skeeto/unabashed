set -- a b c
shift 5
echo "a st=$? n=$#"
shift -1
echo "b st=$? n=$#"
shift abc
echo "c st=$? n=$#"
shift 3
echo "e st=$? n=$#"
shift
echo "f st=$? n=$#"
shopt -s shift_verbose
shift
echo "g st=$?"
set -- x
shift 2
echo "h st=$? n=$#"
shift 0
echo "i st=$? n=$#"
f() { shift 3; echo "in f st=$? n=$#"; }
f p q
set -- x y
shift -- 1
echo "dd st=$? n=$#"
echo end
