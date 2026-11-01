# Command substitution output: trailing newlines removed, then split
# when unquoted.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show $(printf 'a b\n\nc\n\n\n')
show "$(printf 'a b\n\nc\n\n\n')"
show $(printf '\n\n')
show "$(printf '\n\n')"
show $(:)
show "$(:)"
show x$(printf ' a ')y
show `printf ' p q '`
show "`printf ' p q '`"
IFS=:
show $(printf 'a:b::c:')
show $(printf ' a b ')
unset IFS
show $(echo '*')
show "$(echo "a  b")"
