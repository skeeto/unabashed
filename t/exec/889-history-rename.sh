# history -w and HISTFILESIZE replace the file by writing a new one
# beside it and renaming it over: the new file has mode 0600, a
# read-only file in a writable directory is replaced, and a symbolic
# link stays one, its target replaced
set -o history
echo one
echo two
: > h1
chmod 644 h1
history -w h1; echo "st $?"
ls -l h1 | cut -c1-10
: > h2
chmod 444 h2
history -w h2; echo "st $?"
ls -l h2 | cut -c1-10
cat h2 | wc -l
: > t3
ln -s t3 h3
history -w h3; echo "st $?"
ls -l h3 | cut -c1-10
wc -l < t3
ls
HISTFILE=h1
HISTFILESIZE=1
ls -l h1 | cut -c1-10
cat h1
mkdir ro
: > ro/h
chmod 555 ro
history -w ro/h; echo "st $?"
history -a ro/h; echo "st $?"
chmod 755 ro
ls ro
rm -r h1 h2 h3 t3 ro
