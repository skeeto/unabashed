# vos
# TZ reaches a native child only if it is ':...' or alphanumerics and -+:
for tz in UTC EST5EDT UTC+3 :Europe/Paris Europe/Paris /usr/share/zoneinfo/UTC 'A B' ''; do
    TZ=$tz ARGV_SHOW=TZ argv
done
