# vos
# pwd -W prints the current directory as a forward-slash Windows path
pwd -W
(cd / && pwd -W)
(cd /usr/bin && pwd -W)
(cd /bin && pwd -W)
(cd /etc && pwd -W)
(cd /tmp && pwd -W)
(cd /c && pwd -W)
(cd /c/Windows && pwd -W)
(cd 'C:\Windows\System32' && pwd -W)
(cd /usr && pwd -W)
pwd -W extra; echo "status $?"
pwd -LW; echo "status $?"
pwd -PW; echo "status $?"
