# The history list in a script: set -o history, cmdhist joining,
# HISTSIZE, HISTCONTROL, HISTIGNORE, and history -c -d -s -p
set -o history
echo one
for i in a b
do
  echo $i
done
f() {
  echo in f
}
cat <<END
here
END
history
HISTCONTROL=ignoreboth
 echo hidden
echo dup
echo dup
HISTIGNORE='ls*:&'
ls -d /
history 4
history -d 2
history -d -2--1
history -d 99; echo "st $?"
history -d 1-x; echo "st $?"
history -d; echo "st $?"
history -x; echo "st $?"
history -an; echo "st $?"
history abc; echo "st $?"
history -s pushed one; history -s pushed two
history 3
history -p 'echo !!' '!pushed:2' '!!:z'; echo "st $?"
HISTSIZE=3
history
history -c
history
