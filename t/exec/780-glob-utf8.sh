# In a UTF-8 locale ? and brackets match whole characters, in names and
# in patterns; in the C locale they match bytes.
cd /tmp && mkdir u8 && cd u8
touch 'é' 'ab' 'aé' 'ñx' 'z'
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
LC_ALL=C.UTF-8
show '?'      ?
show '??'     ??
show 'a?'     a?
show '[é]'    [é]
show '[!a-z]*' [!a-z]*
show '[ñé]?'  [ñé]?
show '*é'     *é
LC_ALL=C
show 'C ?'    ?
show 'C ??'   ??
show 'C a?'   a?
show 'C a??'  a??
LC_ALL=C.UTF-8
x='aéb'
echo "${x/?b/_}" "${x//[é]/E}" "${#x}"
case é in ?) echo one char ;; *) echo not one ;; esac
LC_ALL=C
case é in ?) echo one byte ;; ??) echo two bytes ;; esac
