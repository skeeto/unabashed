# help: the topics' synopses, the listing's layout, matching rules,
# statuses and messages, and --help on builtins (descriptions are our
# own words, so only their first line's shape is checked)
help | sed 1,8d
COLUMNS=60 help | sed 1,8d | head -5
COLUMNS=200 help | sed 1,8d | head -3
help -s '*' ; echo "status $?"
help -s rea; help -s for; help -s '[' '[[' '{'
help -s 'r*' 'z*'; echo "status $?"
help -d 'rea*' x | cut -d' ' -f1-2
help zz; echo "status $?"
help -x; echo "status $?"
help -s -- -x; echo "status $?"
help -- | sed -n 9p
for b in . alias cd shift read wait source typeset help; do
    $b --help | head -1; echo "$b ${PIPESTATUS[0]}"
done
for b in : echo true false test; do $b --help; echo "$b $?"; done
[ --help; echo "[ $?"
builtin help -s shift; command help -s help
enable -n echo; help | sed 1,8d | grep -F '*'; enable echo
help true >&-; echo "status $?"
help -m true | sed -n '1p;4,5p;7p;/^SEE/,/^IMPL/p'
help -d true false | cut -d' ' -f1,2
help >&-; echo "status $?"
