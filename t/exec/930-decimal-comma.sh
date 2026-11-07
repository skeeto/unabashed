# LC_NUMERIC's decimal point in printf, time and EPOCHREALTIME
exec 2>&1
export LANG=de_DE.UTF-8 LC_MESSAGES=C  # messages stay untranslated
printf '%.4f %g %e\n' 1 2,5 3
printf '%.2f\n' 1.5; echo $?
printf '%.2f\n' 1,25e1
LANG=C printf '%.4f\n' 1
(LANG=C; printf '%.4f\n' 1)
LC_NUMERIC=C printf '%.2f\n' 2.5
LC_ALL=C printf '%.2f\n' 2.5
LC_ALL=en_US.UTF-8 LC_NUMERIC=de_DE.UTF-8 printf '%.2f\n' 2,5
LANG=fr_FR.UTF-8 printf '%.1f\n' 2
LANG=de_CH.UTF-8 printf '%.1f\n' 2
LANG=pt_BR.UTF-8 printf '%.1f\n' 2
LANG=ja_JP.UTF-8 printf '%.1f\n' 2
LANG=de_DE.UTF-8@euro printf '%.1f\n' 2
LC_NUMERIC=bogus printf '%.1f\n' 2
[[ $EPOCHREALTIME == *,* ]] && echo comma
TIMEFORMAT='%3R %1U %0S'; time :
LANG=C; time :
printf '%.1f\n' 2
