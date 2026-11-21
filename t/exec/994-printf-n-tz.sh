# printf %n may assign TZ, freeing the text the time zone's names were
# taken from: later %(...)T conversions follow the new TZ, as in bash
export TZ=ABCDEFGH0
printf '%(%Z)T %n[%(%Z)T]\n' -1 TZ
echo "TZ=$TZ"
declare -n ref=TZ
export TZ=LONGZONENAME-3
printf '%(%Z %H)T%n [%(%Z)T]\n' 0 ref 0
echo "TZ=$TZ"
