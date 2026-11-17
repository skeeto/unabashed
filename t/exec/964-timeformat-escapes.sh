# TIMEFORMAT's escapes (#12): a precision of up to 6 places, and a
# format that ends after its precision or l rejected, with no report.
# Digits are masked, as times vary. bash names the end of the format as
# a NUL byte, which is dropped here: unabashed leaves the quotes empty.
for f in '%R' '%0R' '%1R' '%6R' '%9R' '%lR' '%7lR' '%3lU|%S' 'a%%b%' \
         '%l' '%3' '%0' '%9' '%3l' 'x%3' '%x' '%l%' '%5%'; do
    TIMEFORMAT=$f
    { time :; } 2>&1 | tr -d '\000' | tr 0-9 D
    echo "status $?"
done
