# printf: no positional arguments, length modifiers ignored, bad characters
exec 2>&1
printf '%1$s|\n' a b; echo $?
printf '%2$*1$d|\n' 5 3; echo $?
printf '%*1$d|\n' 5 3; echo $?
printf '%I d|\n' 5; echo $?
printf '%.3$f|\n' 5; echo $?
printf '%ls %lc|\n' abc d
printf '%hhd %lld %jd %zd %td %Lf %qd|\n' 1 2 3 4 5 6 7; echo $?
printf '%v\n' 1; echo $?
