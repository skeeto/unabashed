# field splitting of unquoted expansions with various IFS
p() { printf '<%s>' "$@"; echo; }
v=' a  b:c::d: '
p $v
IFS=:; p $v
IFS=' :'; p $v
IFS=': '; p $v
IFS='ab'; p $v
IFS=; p $v
w='::x'; IFS=:; p $w
w='x::'; p $w
w=':'; p $w
w=' : '; IFS=' :'; p $w
w='a : : b'; p $w
