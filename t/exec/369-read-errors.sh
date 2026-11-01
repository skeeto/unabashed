# option and argument errors from read
read -z x <<<"a"; echo "status $?"
read 1bad <<<"a"; echo "status $?"
read ok bad-name <<<"a b"; echo "status $? ok=[$ok]"
read -n abc x <<<"a"; echo "status $?"
read -n -2 x <<<"a"; echo "status $?"
read -d; echo "status $?"
read -u; echo "status $?"
read -- x <<<"dashdash"; echo "status $? x=$x"
readonly ro=1
read ro <<<"new"; echo "status $? ro=$ro"
read -t x y <<<"a"; echo "status $?"
