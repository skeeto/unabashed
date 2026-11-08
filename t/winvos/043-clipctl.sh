# personality: msys
# Hand-checked baseline: the vos clipboard as clipctl, a fake Windows
# program, drives it: empty at first, text set from standard input (up
# to a NUL, as Windows stores UTF-16), non-text contents, and the count
# of sets. Later specs read and write it through /dev/clipboard.
clipctl get | od -c
printf 'a\r\nb' | clipctl set
clipctl get | od -c
printf 'x\0y' | clipctl set
clipctl get | od -c
clipctl nontext
clipctl get | od -c
clipctl count
clipctl bogus; echo "status $?"
