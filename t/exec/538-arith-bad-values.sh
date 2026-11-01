v=1a
(( v )); echo "1a status $?"
w='1 2'
(( w )); echo "1 2 status $?"
q='"3"'
(( q )); echo "dq status $?"
t='@'
(( t )); echo "at status $?"
(( '1' + 2 )); echo "single quotes status $?"
(( "1" + 2 )); echo "double quotes status $?"
echo end
