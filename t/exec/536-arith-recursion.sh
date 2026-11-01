a=b
b=c
c=7
echo "chain $(( a ))"
r=r
(( r )); echo "self status $?"
p=q q=p
(( p + 1 )); echo "mutual status $?"
echo end
