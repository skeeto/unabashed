# A file the kernel will not run, not a script either (a NUL in its
# head): bash's check_binary_file says so, status 126
printf 'abc\0def\n' > b1; chmod +x b1
printf '\177ELF\2\1\1\0junk' > b2; chmod +x b2
./b1; echo "st $?"
./b2; echo "st $?"
./b1 | cat; echo "st ${PIPESTATUS[*]}"
./b1 & wait $!; echo "st $?"
