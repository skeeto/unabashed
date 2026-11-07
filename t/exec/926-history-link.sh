# Replacing a history file through a symbolic link writes beside its
# target and renames over the target (mode 0600); the target is read as
# written, from the working directory, as readline does. A file that
# is no regular file, or missing, is written in place
set -o history
echo one
echo two
mkdir -p /tmp/hl/d; cd /tmp/hl
: > t1; chmod 644 t1; ln -s t1 h1
history -w h1; echo "st $?"
ls -l t1 | cut -c1-10; ls -l h1 | cut -c1-10; wc -l < t1
: > d/t2; chmod 644 d/t2; ln -s t2 d/h2
history -w d/h2; echo "st $?"
ls -l d/t2 | cut -c1-10; wc -l < d/t2
ls -l t2 | cut -c1-10; wc -l < t2
history -w /dev/null; echo "st $?"
ls -l /dev/null | cut -c1
history -w new; echo "st $?"
ls -l new | cut -c1-10
printf '%s\n' a b c d > t3; chmod 644 t3; ln -s t3 h3
HISTFILE=h3
HISTFILESIZE=2
ls -l t3 | cut -c1-10; ls -l h3 | cut -c1; cat t3
ls; ls d
