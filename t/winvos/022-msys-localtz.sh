# personality: msys
# localtz: EST5EDT,M3.2.0,M11.1.0
# Hand-checked baseline, not recorded from an oracle (M5a step 9). With
# TZ unset, local time is the system zone (the vos stands in for
# GetTimeZoneInformation, which the shell renders as a POSIX TZ string
# with capitals, as Cygwin). A TZ the shell cannot use is GMT, as in
# Cygwin; Git has no zoneinfo, so a zone name is GMT too. The system
# zone is not exported to children.
unset TZ
printf '%(%F %T %Z %z)T\n' 1700000000 1690000000
p='\D{%H:%M %Z}'
echo "${p@P}"
TZ=JST-9 printf '%(%T %Z)T\n' 1700000000
TZ=garbage printf '%(%T %Z)T\n' 1700000000
TZ=America/New_York printf '%(%T %Z)T\n' 1700000000
TZ= printf '%(%T %Z)T\n' 1700000000
ARGV_SHOW=TZ argv
