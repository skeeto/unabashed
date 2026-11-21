# %(...)T with length modifiers, after flags, width and precision, still
# formats in the exported zone
export TZ=EST5
printf '%l(%H %Z)T|\n' 0
printf '%L(%H %Z)T|%h(%H %Z)T|%j(%H)T|%t(%H)T|%z(%H)T\n' 0 0 0 0 0
printf '%-8l(%H %Z)T|%8.2hl(%H %Z)T|%#0ll(%H)T|%*.*z(%Z)T|\n' 0 0 0 6 2 0
printf -v v '%l(%H %Z)T' 0; echo "$v"
printf -v v '%5lL(%Z)T' 0; echo "$v"
TZ=JST-9 printf 'temporary: %l(%H %Z)T\n' 0
