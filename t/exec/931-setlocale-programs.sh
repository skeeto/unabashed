# Locale warnings around programs: undone right after the spawn, never
# in a stage that ends in its program; an assigned LC_ALL quiets the rest
exec 2>&1
LC_ALL=zh_TW.big5
echo a | LC_ALL=C od -b | head -1
echo b
LC_ALL=zh_TW.big5 bash -c 'echo ok 7'
LC_CTYPE=ru_RU.CP1251 printf 'x\n' | cat
LC_ALL=C od -b </dev/null; echo c
LC_ALL=C nosuch; echo d
unset LC_ALL
LC_CTYPE=ru_RU.CP1251 printf 'y\n' | cat
export LC_ALL=en_US.UTF-8
LC_CTYPE=ru_RU.CP1251 printf 'z\n' | cat
LC_ALL=en_US.UTF-8 bash -c 'LC_CTYPE=ru_RU.CP1251 printf "w\n"'
echo end
