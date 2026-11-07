# Assigning a locale variable that names no installed locale warns
exec 2>&1
LC_ALL=zh_TW.big5; echo 1
unset LC_ALL; LANG=foo; echo 2
LANG=C; LC_CTYPE=ru_RU.CP1251; echo 3
LC_NUMERIC=xx_YY; LC_TIME=de_DE; LC_COLLATE=en_US; echo 4
LC_ALL=zh_TW.big5 printf '%s\n' 5
x() { local LC_ALL=qq; echo 7; }; x
LANG=en_US.UTF-8 LC_ALL=C; echo 8
LC_ALL=ja_JP.eucJP true; LANG=de_DE.utf8; LC_ALL=en_US.utf8; echo 9
set -o posix; LC_ALL=zz; echo 10

B=env; LC_ALL=zh_TW.big5 $B true; echo 11
LC_ALL=zh_TW.big5 command env true; echo 12
LC_ALL=zh_TW.big5 env true; echo 13
LC_ALL=C.UTF-8 env true; echo 14
