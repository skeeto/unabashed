# glibc's matching rules, extensions and quirks, through =~
t() {
    if [[ $2 =~ $1 ]]; then
        local i out=
        for ((i = 0; i < ${#BASH_REMATCH[@]}; i++)); do out+="<${BASH_REMATCH[i]}>"; done
        printf '%-18s %-8q %s\n' "$1" "$2" "$out"
    else
        printf '%-18s %-8q no %d\n' "$1" "$2" $?
    fi
}
t '(|a)' a
t '(a|)' a
t '(a*)*' b
t '(a?)*' aa
t '(a|ab)(c|bcd)(d*)' abcd
t '(wee|week)(knights|night)' weeknights
t '(a*)+' b
t '((a)|b)*' ab
t '(x?){2}' x
t '(.)(.)?(.)?' ab
t 'x*' zz
t '\w+' '--ab_1--'
t '\W+' 'ab--cd'
t '\s+\S' 'a  b'
t '\bab' 'xab ab'
t 'ab\b' 'abx ab'
t '\<a.' 'ba ab'
t '.a\>' 'ab ba'
t '\`a' 'aa'
t "a\\'" 'aa'
t 'a*\B' 'ca.'
t 'b*\B' '_b'
t '\B' '.ab'
t '[]a]+' 'x]a]'
t '[^]a]' ']]b'
t '[a-]+' 'x-a-'
t '[[.-.]]' 'a-b'
t '[[=a=]]' 'bab'
t '[[:digit:][:punct:]]+' 'x1,2y'
t 'a{2}' 'aaa'
t 'a{,2}' 'aaa'
t 'a{0}b' 'ab'
t 'a{1,3}?' 'aaaa'
t '(a){0}\1' 'a'
t '(a*)\1' 'aaaa'
t '(a|b)*\1' 'abb'
t '(a?){2}\1' 'aa'
t ')' 'a)b'
t '}' 'a}b'
nl=$'\n'
t "a\$." "a${nl}b"
t "(a)\$." "a${nl}b"
t "(.)^b" "a${nl}b"
t '^b' "a${nl}b"
t '(^a)+' 'aa'
t '(\ba)+' 'aa'
t '(^a|b)+' 'aab'
t '(a(b)?)+' 'aba'
t '(a|b){2,3}' 'ababa'
