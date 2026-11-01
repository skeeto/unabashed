# set -e and command substitution (inherit_errexit off by default)
set -e
x=$(false; echo still-runs)
echo "x=$x"
y=$(set -e; false; echo no)
echo "y=[$y]"
shopt -s inherit_errexit
z=$(false; echo no2)
echo "z=[$z]"
shopt -u inherit_errexit
echo "$(false) unaffected"
echo end
v=$(exit 2)
echo never
