x=global
inner() { unset x; echo "inner after unset: [${x-unset}]"; }
outer() { local x=outer; inner; echo "outer after: [${x-unset}]"; }
outer
echo "top: [${x-unset}]"
g=1
kill_global() { unset g; }
kill_global
echo "g: [${g-unset}]"
two() { local t=2; inner2; echo "two: [${t-unset}]"; }
one() { local t=1; two; echo "one: [${t-unset}]"; }
inner2() { unset t; echo "inner2 sees: [${t-unset}]"; unset t; echo "inner2 again: [${t-unset}]"; }
t=top
one
echo "t: [${t-unset}]"
