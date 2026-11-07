# Regexes in UTF-8: bracket classes take multibyte characters as
# iswctype does, as do \w, \s and the word anchors (an invalid byte
# counting as its Latin-1 character), and
# nocasematch compares characters raised, as glibc's REG_ICASE does
# (ı and ſ raise to ASCII I and S)
exec 2>&1
LC_ALL=C.UTF-8
for s in é É ß € 中 ٣ a; do
    printf '%s:' "$s"
    for k in alpha upper lower punct alnum space print graph; do
        [[ $s =~ ^[[:$k:]]$ ]] && printf ' %s' $k
        [[ $s =~ ^[^[:$k:]]$ ]] && printf ' !%s' $k
    done
    echo
done
[[ 'aéb' =~ a([[:alpha:]])b ]] && echo "m ${BASH_REMATCH[1]}"
[[ 'xÉTÉy' =~ [[:upper:]]+ ]] && echo "m $BASH_REMATCH"
shopt -s nocasematch
[[ ÉTÉ =~ ^été$ ]] && echo "icase eq"
[[ ÉTÉ =~ ^[é]t[é]$ ]] && echo "icase bracket"
[[ ÉTÉ =~ ^[à-ê]+ ]] && echo "icase range $BASH_REMATCH"
[[ Ω =~ ω ]] && echo "icase omega"
shopt -u nocasematch
for re in '^\w+$' '\W' '\<é' 'a\>' '\bé' 'é\B' '\s'; do
    for s in 'aéb' 'é-' $'x\xffy' $'\xff' 'a b' $'a b' $'a　b'; do
        [[ $s =~ $re ]] && r=${BASH_REMATCH[0]} || r='-'
        printf '%s %q %q\n' "$re" "$s" "$r"
    done
done
shopt -s nocasematch
t() { [[ $1 =~ $2 ]] && echo "$1 =~ $2: [$BASH_REMATCH]" || echo "$1 =~ $2: no"; }
t é '^[[:lower:]]$'
t É '^[[:lower:]]$'
t é '^[[:upper:]]$'
t ǆ 'ǅ'
t ǅ 'ǆ'
t ı 'i'
t ı 'I'
t I 'ı'
t i 'ı'
t ſ 's'
t ß 'ẞ'
t xÉy 'x.y'
t 'aÉÉb' 'a(é+)b'
t € '[^[:alpha:]]'
t 'Straße' 'STRASSE'
shopt -u nocasematch
t € '[^[:alpha:]]'
t 'a中b' 'a[[:alpha:]]b'
t 'a中b' 'a[^[:alpha:]]b'
t 'x٣' '[[:digit:]]'
