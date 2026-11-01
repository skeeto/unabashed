v=global
inner() { echo "inner v=$v"; }
outer() { echo "outer v=$v"; v=inner-tmp inner; echo "outer again v=$v"; }
v=tmp outer
echo "top v=$v"
loc() { local v=loc; v=tmp2 inner; echo "loc v=$v"; }
loc
setv() { v=set-in-func; }
v=tmp3 setv
echo "after setv v=$v"
unset v
show_u() { echo "u=[${u-unset}]"; }; u=tmp4 show_u; show_u
inh() { local w; echo "local w=[${w-unset}]"; w=changed; echo "w=$w"; }
w=tmpw inh
echo "w after=[${w-unset}]"
