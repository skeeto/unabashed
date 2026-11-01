readonly R=1
f() { echo "f runs R=$R"; }
R=2 f
echo "status $?"
R=3 echo "echo runs"
echo "status $?"
R=4 :
echo "status $?"
echo "R=$R"
