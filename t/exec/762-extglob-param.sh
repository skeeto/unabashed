# extglob patterns in ${x#p} ${x##p} ${x%p} ${x%%p} and ${x/p/r}.
shopt -s extglob
x=000123000
echo "${x##+(0)}" "${x%%+(0)}" "${x#?(0)}" "${x%*(0)}"
y='aXbXXcXXXd'
echo "${y//+(X)/-}" "${y/+(X)/-}" "${y//@(X|XX)/-}" "${y//!(X)/_}"
echo "${y/#a*(X)/^}" "${y/%*(X)d/$}"
z='foo.tar.gz'
echo "${z%.@(gz|bz2)}" "${z%%.*([a-z])}" "${z##*.}" "${z#!(.)}"
w='   lead and trail   '
echo "[${w##+([[:space:]])}]" "[${w%%+([[:space:]])}]"
v=abcabc
echo "${v//?(b)/.}" "${v//*(c)/.}" "${v/@(bc|b)/}"
a=(one.c two.h three.o)
echo "${a[@]%.@(c|h)}" "${a[@]/#@(o|t)/X}"
set -- xyz.c abc.h
echo "${@//[a-c]/-}" "${*%.!(c)}"
shopt -s nocasematch
u='ABCabc'
echo "${u//+(b)/_}" "${u#+(a)}"
