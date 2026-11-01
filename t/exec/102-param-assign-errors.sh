# ${N=w} for positional and special parameters cannot assign
set -- a
( echo "${1=x}" ); echo "status $?"
( echo "${2=x}" ); echo "status $?"
( echo "${2:=x}"; echo not reached ); echo "status $?"
( echo "${#=x}" ); echo "status $?"
( readonly r; echo "${r=x}"; echo after ); echo "status $?"
( readonly r2=; echo "${r2:=x}"; echo after ); echo "status $?"
echo end
