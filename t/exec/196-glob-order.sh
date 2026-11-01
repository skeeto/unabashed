# Glob results sort in the C locale's byte order.
cd /tmp
for f in b B a A 1 _ Z z aa a_ a- 'a b' ab; do : > "$f"; done
printf '[%s]' *; echo
printf '[%s]' a*; echo
printf '[%s]' ?; echo
