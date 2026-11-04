# What a glob of / lists in msys mode: the root's own entries plus
# mount points and the virtual /dev and /proc (recorded, not guessed)
echo /*
echo /.*
(cd / && echo *)
echo /?
echo /[a-z]*
echo /dev/n*
echo /proc/s*
