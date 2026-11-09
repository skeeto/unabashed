# Pattern words of plain text, with and without pattern characters, in
# every parameter pattern operator, case and [[ ]], then extglob,
# nocasematch, and quoted or expanded pieces beside them
s='ab:cd-ef]gh!x^y@z+w|v*u?t[s'
for pat in '*:' ':*' '?' '??' '[ab]*' '*[!x]' '[[:alpha:]]*' '*]*' '*-*' '!*' '^*' '*@' '+*' '*|*' '\**' '*\?*' '*[*]*' '[!a]*' '*[' 'a]' '*[]]*' '*[^b]' '[a-c]?' '*[[]*'; do
    eval 'printf "%s|" "${s#'"$pat"'}" "${s##'"$pat"'}" "${s%'"$pat"'}" "${s%%'"$pat"'}" "${s/'"$pat"'/R}" "${s//'"$pat"'/R}" "${s/#'"$pat"'/R}" "${s/%'"$pat"'/R}"'
    echo
    eval 'case $s in '"$pat"') echo "case $pat yes";; *) echo "case $pat no";; esac'
    eval '[[ $s == '"$pat"' ]] && echo "cond $pat yes" || echo "cond $pat no"'
done
shopt -s extglob
for pat in '*:' '+*' '@*' '!*' '*!' '?*' '*+'; do
    eval 'printf "%s|" "${s#'"$pat"'}" "${s%%'"$pat"'}" "${s//'"$pat"'/R}"'; echo
done
shopt -s nocasematch
for pat in 'AB*' '*CD*' '[A]*'; do
    eval 'printf "%s|" "${s#'"$pat"'}" "${s/'"$pat"'/R}"'; echo
    eval 'case $s in '"$pat"') echo "nc $pat yes";; *) echo "nc $pat no";; esac'
done
shopt -u nocasematch
x='a*b'; echo "${x#a*}" "${x#"a*"}" "${x#a\*}" "${s#*"]"}"
p='*:'; echo "${s#$p}" "${s#"$p"}"
echo "${s#~}" "${HOME#~}"x
