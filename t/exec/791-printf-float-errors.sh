# printf: what strtold accepts, and the messages bash gives
for a in x 08x 0x1g 1e99999 1e-99999 1e-4940 '' ' 3' '3 ' 1e 1e+ . 0x \
         0x.p1 'nan(' 'nan(x_1)' infinity infinit +.5 -.5e1 00012.500; do
    printf '[%s] [%f]\n' "$a" "$a"
done 2>&1
printf '%f\n' x; echo status $?
printf '%e\n' 1e999999; echo status $?
