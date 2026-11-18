# select reads its reply as the read builtin does without options:
# backslashes escape and join lines, blanks stay in REPLY, and the end
# of input, even after part of a line, ends the loop with REPLY set
printf '1\\\n2 \\x \n' | { select x in a b; do echo "[$REPLY] $x"; done; }
REPLY=x
select x in a; do echo body; done </dev/null
echo "st $? [$REPLY]"
printf 1 | { select x in a; do echo body; done; echo "st $? [$REPLY]"; }
printf '\n\n2\n' | { select x in a b; do echo "[$REPLY] $x"; break; done; }
