# vos
# cd to drive roots in every spelling; PWD takes the POSIX form
cd /c && pwd && echo "$PWD"
cd C:/ && pwd
cd 'C:\' && pwd
cd c:/ && pwd
cd C: && pwd
cd /C && pwd
cd C:/Windows && pwd
cd 'C:\Windows' && pwd
cd /c/Windows/ && pwd
cd 'c:\windows\System32' && pwd
cd C:/Windows/../Windows && pwd
