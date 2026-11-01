for 1x in a; do echo no; done
echo "for st=$?"
unset 1x
echo "unset st=$?"
unset -v 'a b'
echo "unset-v st=$?"
export 1x=2
echo "export st=$?"
readonly 'a b'=1
echo "readonly st=$?"
declare 3=4
echo "declare st=$?"
read 1x <<< data
echo "read st=$?"
f() { local 9z=1; echo "local st=$?"; }
f
x-y=1
echo "assign-as-cmd st=$?"
unset -f 'not valid'
echo "unset-f st=$?"
export a=1 2b=2 c=3
echo "multi st=$? a=$a c=$c"
echo end
