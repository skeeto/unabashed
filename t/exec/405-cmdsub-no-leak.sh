# Assignments and other state inside $( ) do not leak
a=outer
x=$(a=inner; echo $a)
echo "$x $a"
f() { echo f-outer; }
y=$(f() { echo f-inner; }; f)
echo "$y"
f
z=$(cd /tmp; pwd)
echo "$z"
pwd
w=$(set -u; echo opt)
echo "$w"
echo "${unset_var-ok}"
q=$(unset a; echo "${a-gone}")
echo "$q $a"
export E=1
r=$(export E=2; echo $E)
echo "$r $E"
