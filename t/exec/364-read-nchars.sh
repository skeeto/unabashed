# read -n and -N
read -n 3 x <<<"abcdef"; echo "status $? x=$x"
read -n 10 x <<<"ab"; echo "status $? x=$x"
read -n 3 x <<<"a
bc"; echo "x=[$x]"
read -N 3 x <<<"a
bc"; echo "x=[$x]"
read -N 10 x <<<"ab"; echo "status $? x=[$x]"
read -n 0 x <<<"abc"; echo "status $? x=[$x]"
read -n 4 a b <<<"x y z"; echo "a=[$a] b=[$b]"
read -N 4 a b <<<"x y z"; echo "a=[$a] b=[$b]"
read -N 4 x <<<"  ab"; echo "x=[$x]"
read -n 4 x <<<"  ab"; echo "x=[$x]"
read -n 2 x <<<'\ab'; echo "x=[$x]"
read -rn 2 x <<<'\ab'; echo "x=[$x]"
read -n3 x <<<"12345"; echo "x=$x"
