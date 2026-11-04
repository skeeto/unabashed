# vos
# Failing cd in each spelling: the message names the path as typed
cd /no/such/dir; echo "status $?"
cd C:/no/such/dir; echo "status $?"
cd 'C:\no\such\dir'; echo "status $?"
cd test.sh; echo "status $?"
cd ''; echo "status $?"
echo "${PWD##*/}"
