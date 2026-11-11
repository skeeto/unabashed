# personality: native
# Hand-checked baseline: native mode refuses a working directory that a
# program cannot start in, as msys mode does (049).
base=$PWD
x=$(printf "%0$((258 - ${#base} - 1))d" 0)
busybox mkdir "$x" "${x}1"
cd "$x" && argv fits; echo "258 $? ${#PWD}"
cd ../"${x}1" && argv long; echo "259 $?"
( argv sub ); echo "subshell $?"
cd ..
busybox mkdir gone && cd gone && (cd .. && busybox rmdir gone)
argv gone; echo "gone $?"
argv stage | busybox cat; echo "pipeline $?"
argv bg & wait $!; echo "background $?"
echo "comsub [$(argv comsub)] $?"
echo x >f; echo "redirection $?"
echo builtin; pwd
cd .. && argv back; echo "back $?"
