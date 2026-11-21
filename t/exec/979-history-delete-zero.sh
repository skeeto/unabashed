# history -d -0 is out of range, as one past the newest entry: with an
# empty list, after an earlier deletion, and before a new entry. The
# range forms read 0 and -0 as the first entry, as bash does.
history -d -0; echo "status $?"
history -s a
history -s b
history -s c
history -d 1
history -d -0; echo "status $?"
history -s d
history
for d in -0 0 4 -4 -00 +0 -1--0 1-0 -0-1 -0--0 -3-3 2-1; do
  history -c
  history -s a; history -s b; history -s c
  history -d "$d"; echo "$d: status $?"
  history
done
history -c
history -d 1-0; echo "status $?"
history -d -0-0; echo "status $?"
