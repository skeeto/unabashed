show() { echo "VAR=[${VAR-unset}]"; declare -p VAR; }
VAR=x show
echo "after: [${VAR-unset}]"
VAR=orig
VAR=tmp show
echo "after: $VAR"
mod() { VAR=modified; echo "in mod: $VAR"; }
VAR=tmp mod
echo "after mod: $VAR"
loc() { local VAR=inner; echo "in loc: $VAR"; }
VAR=tmp loc
echo "after loc: $VAR"
A=1 B=2 C=$VAR show
two() { echo "A=$A B=$B"; }
A=1 B=2 two
echo "A=[${A-unset}] B=[${B-unset}]"
