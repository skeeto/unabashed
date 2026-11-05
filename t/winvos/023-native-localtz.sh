# personality: native
# localtz: <+0530>-5:30
# Hand-checked baseline, not recorded from an oracle (M5a step 9). With
# TZ unset, local time is the system zone, here one without capitals
# in its name (rendered numerically) and no daylight time. Native mode
# has no zoneinfo: TZ is a POSIX string, and a zone name is a name at
# offset 0 (what busybox-w32 prints here is unverified). The system
# zone is not exported to children.
unset TZ
printf '%(%F %T %Z %z)T\n' 1700000000
p='\D{%H:%M %z}'
echo "${p@P}"
TZ=EST5EDT,M3.2.0,M11.1.0 printf '%(%T %Z)T\n' 1690000000
TZ=America/New_York printf '%(%T %Z %z)T\n' 1700000000
ARGV_SHOW=TZ argv
