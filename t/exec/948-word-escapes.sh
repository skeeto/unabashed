# Words of literal text, quotes and backslashes: escapes in double quotes
# and outside them, next to quotes, before newlines, in aliases, case
# patterns and test operands
shopt -s expand_aliases
p() { printf '%s' "$#"; printf ' <%s>' "$@"; echo; }
cd /tmp && mkdir -p escw && cd escw && : >ab && : >'a b' && : >'*'
p "test\n" "a\"b" "a\\b" "a\$b" "a\`b" "a\'b" 'x\y' "\\" "\\\\" "\a\b\c"
p a\ b \* \? \[ \~ \{a,b\} a\$b a\\b \"q\" \'s\' \# a\;b
p "a"\ "b" \"'x'\" a"\n"b x\ "\$"\ y 'it'\''s' "\\$"
p "\$HOME" \$HOME "\`echo hi\`" ab\* a\ b\* "ab"*
p "a\
b" a\
b
p "" '' \  "\ " "\\ " \\\\
echo "test\n"; printf "test\n"; printf "%s|%s\n" "a\tb" a\tb
x=1; p "\$x" "\\$x" \$x \\$x
p "a\${x}b" a\${x}b "\${x:-d}"
alias e1='p "al\ias"'
e1
case "a\b" in "a\b") echo cq;; esac
case 'a\b' in a\\b) echo cu;; esac
[ "a\b" = 'a\b' ] && echo t1
f() { p "$1" "\n" \n; }; f "\t"
cd /tmp && rm -f escw/ab 'escw/a b' 'escw/*' && rmdir escw
