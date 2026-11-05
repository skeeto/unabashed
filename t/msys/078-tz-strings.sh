# vos
# TZ strings in the shell's own time formatting: an invalid or empty
# TZ, or a zone name without zoneinfo (Git ships none), is GMT
for tz in EST5EDT EST5 EST Europe/Paris :Europe/Paris garbage '' : JST-9 \
          '<+0530>-5:30' Abc1Xyz 'EST5EDT,M3.2.0/2,M11.1.0/2' 'XST5XDT4,J60,J300/1'; do
    export TZ=$tz
    printf '[%s] %(%F %T %Z %z)T %(%Z %z)T\n' "$tz" 1700000000 1690000000
done
p='\D{%Z %z}'
TZ=garbage
echo "prompt ${p@P}"
