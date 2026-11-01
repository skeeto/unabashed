# where unbraced parameter names end
p() { printf '<%s>' "$@"; echo; }
x=X; x_y=XY; set -- one two
p "$x_y" "$x-y" "${x}_y" "$x.y" "$x[0]" "$x:" "$x{"
p "$1x" "$12" "$@x" "$#x" "$?x" "$0x"
p "$" "a$" "$ x" "$%" "$=" "$/"
p $ "x$"
p "\$x" '$x' "$\x"
