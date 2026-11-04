# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a step 5).
# The msys persona's virtual files: /dev/null and the devices, reserved
# DOS names, /proc/self, /proc/cygdrive. A name finds name.exe. HOME
# matches in any case in \w and \W. The root lists the mount points.
# msyspath off renders the native forms.
echo hi >/dev/null && echo null ok
cat /dev/null; echo "cat null $?"
echo hi >NUL && echo NUL ok
[ -c /dev/null ] && echo null is a device
[ -d /dev ] && echo /dev is a directory
echo /dev/std*
[ -e /proc/self ] && echo /proc/self exists
read -r x </proc/self/exename && echo "$x"
read -r x </proc/self/winexename && echo "$x"
read -r x </proc/self/comm || echo no comm
[ -d /proc/cygdrive ] && echo cygdrive is a directory
cd /proc/cygdrive/c && pwd
cd /
echo *
[ -e /usr/bin/ls ] && echo ls by suffix
[ -x /usr/bin/ls ] && echo ls is executable
[ -e /usr/bin/nosuch ] || echo no nosuch
HOME=/c/Users/Oracle
cd /c/users/oracle
PS1='\w|\W' && echo "${PS1@P}"
cd /c/users/oracle/AppData
PS1='\w|\W' && echo "${PS1@P}"
shopt -u msyspath
echo "$PATH"
echo "$PWD"
cd C:/work && pwd
shopt -s msyspath
echo "$PWD"
