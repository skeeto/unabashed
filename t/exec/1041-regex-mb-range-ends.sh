# A multibyte range endpoint has no collation value in C.UTF-8, which
# has no collation rules: glibc rejects the range rather than compare
# code points. In C the endpoints are bytes, and so is the subject.
LC_ALL=C.UTF-8
[[ é =~ ^[à-ÿ]$ ]]; echo "1 $?"
[[ б =~ ^[а-я]$ ]]; echo "2 $?"
[[ é =~ ^[a-é]$ ]]; echo "3 $?"
[[ é =~ ^[[.é.]-ÿ]$ ]]; echo "4 $?"
[[ é =~ ^[é]$ ]]; echo "5 $?"
[[ b =~ ^[a-c]$ ]]; echo "6 $?"
LC_ALL=C
[[ é =~ ^[à-ÿ]$ ]]; echo "7 $?"
[[ é =~ ^[à-ÿ]{2}$ ]]; echo "8 $?"
[[ z =~ ^[a-é]$ ]]; echo "9 $?"
[[ é =~ ^[[.é.]-ÿ]$ ]]; echo "10 $?"
