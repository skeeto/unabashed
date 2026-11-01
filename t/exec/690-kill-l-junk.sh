# kill -l with numbers that run into other bytes, from the fuzzer
kill -l $'2\n n=abc\n ech\207 '; echo $?
kill -l 9x 1234567890123456789012345; echo $?
kill -l 130 RTMIN+3 15; echo $?
