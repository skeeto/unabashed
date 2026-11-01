# indirection through special parameter names
p() { printf '<%s>' "$@"; echo; }
set -- x 'y z'
a='@'; p "${!a}"
b='*'; p "${!b}"
c='#'; p "${!c}"
d='0'; p "${!d}"
false; e='?'; p "${!e}"
