# In a UTF-8 locale, character classes and case cover all of Unicode,
# as glibc's iswctype, towupper and towlower in C.UTF-8 do; in the C
# locale, and for invalid bytes, only ASCII letters have classes and case
exec 2>&1
classes() {
    for c in a Z é É ß ǅ Ω ω 5 ٣ '€' ' ' $'　' $' ' ¹ ½ $'​' 中 $'\xe9'; do
        printf '%s:' "$c"
        for k in alpha upper lower digit alnum punct space blank print graph cntrl xdigit word; do
            case $c in [[:$k:]]) printf ' %s' "$k" ;; esac
        done
        echo
    done
}
convert() {
    x='héllo wörld ǅemal straße ıi İ ÿ'
    echo "${x^^} | ${x,,} | ${x^} | ${x~~} | ${x~}"
    echo "${x^^[éö]} | ${x^^[[:alpha:]]}"
    echo "${x@U} | ${x@L} | ${x@u}"
    declare -u u=$x; declare -l l=ÉCOLE; declare -c c=éCOLE
    echo "$u | $l | $c"
    b=$'caf\xe9 \xc3'; echo "${b^^}" | od -An -c | tr -s ' '
    shopt -s nocasematch
    [[ ÉTÉ == été ]] && echo "nocase eq"
    [[ ÉTÉ == [é]T* ]] && echo "nocase bracket"
    case Straße in STRASSE) echo "folded ss" ;; strAẞE) echo "sharp s" ;; *) echo "no" ;; esac
    shopt -u nocasematch
    y='aéb€c'; echo "${y//[[:alpha:]]/.} ${y//[![:punct:]]/_}"
}
echo "== C.UTF-8"
LC_ALL=C.UTF-8
classes
convert
echo "== C"
LC_ALL=C
classes
convert
