# REPLY, variable assignment details, and read inside functions
read <<<"  spaced  out  "; echo "[$REPLY]"
read -r <<<'back\slash'; echo "[$REPLY]"
f() { local v; read v; echo "local v=$v"; }
f <<<"in-func"
echo "global v=[$v]"
read v1 v2 v3 v4 <<<"a b"; echo "[$v1][$v2][$v3][$v4]"
declare -i num
read num <<<"3+4"; echo "num=$num"
unset num
read A_b_9 <<<"name ok"; echo "$A_b_9"
