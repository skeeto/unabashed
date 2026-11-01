(exit); echo "a st=$?"
(false; exit); echo "b st=$?"
(exit 3); echo "c st=$?"
(exit 256); echo "d st=$?"
(exit 257); echo "e st=$?"
(exit -1); echo "f st=$?"
(exit abc; echo after-g); echo "g st=$?"
(exit 1 2; echo after-h); echo "h st=$?"
(exit ''; echo after-i); echo "i st=$?"
(exit ' 4 '); echo "j st=$?"
(exit +5); echo "k st=$?"
(exit 99999999999999999999; echo after-l); echo "l st=$?"
(exit 0x10; echo after-m); echo "m st=$?"
(exit -- 6); echo "n st=$?"
x=$(exit 9); echo "o st=$?"
exit 1 2
echo "top st=$?"
exit foo
echo notreached
