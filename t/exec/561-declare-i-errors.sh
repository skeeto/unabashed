declare -i x=1
x='2 +' ; echo "after bad assign x=$x $?"
echo "next x=$x"
set -u
x=nope; echo "unset ref $? x=$x"
echo end
