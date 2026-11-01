# backslashes in patterns and values
p() { printf '<%s>' "$@"; echo; }
x='a\b\c'
p "${x//\\/\/}" "${x#*\\}" "${x##*\\}" "${x%\\*}"
p "${x/\\b/B}" ${x/'\'/S} "${x/"\\"/D}"
y='a*b?c[d]'
p "${y//[*?]/_}" "${y//\[/<}" "${y/\]/>}" "${y//[][]/|}"
bs='\'; p "${x//$bs/X}" "${x//"$bs"/X}"
pat='\b'; p "${x/$pat/P}"
