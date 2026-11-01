# =~ in a UTF-8 locale: characters, not bytes
LC_ALL=C.UTF-8
[[ é =~ ^.$ ]]; echo 1 $?
[[ é =~ ^..$ ]]; echo 2 $?
[[ xéy =~ x(.)y ]]; echo 3 $? "${BASH_REMATCH[1]}"
[[ é =~ [é] ]]; echo 4 $?
[[ é =~ ^[^a]$ ]]; echo 5 $?
[[ ééé =~ é+ ]]; echo 6 $? "${BASH_REMATCH[0]}"
[[ aéb =~ (é|b)+ ]]; echo 7 $? "${BASH_REMATCH[0]} ${BASH_REMATCH[1]}"
b=$'\xff'
[[ $b =~ ^.$ ]]; echo 8 $?
[[ $b =~ $b ]]; echo 9 $?
re=$'\xa9'; [[ é =~ $re ]]; echo 10 $?
re='[[=é=]]'; [[ é =~ $re ]]; echo 11 $?
[[ € =~ ^[^[:alpha:]0-9]$ ]]; echo 12 $?
LC_ALL=C
[[ é =~ ^..$ ]]; echo 13 $?
[[ é =~ ^.$ ]]; echo 14 $?
