# vos
# HOME arrives as C:\...\home and is imported in POSIX form
echo "$HOME"
echo ~
echo ~/x
x=~/y; echo "$x"
[ ~+ = "$PWD" ] && echo same
cd ~ && pwd && pwd -W
cd && pwd
cd ~/.. && echo "${PWD##*/}"
