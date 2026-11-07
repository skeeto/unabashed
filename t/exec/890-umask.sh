# umask: octal masks up to 07777, leading zeros allowed, larger values
# and other digits out of range; -S and -p
umask 022
umask; echo "st $?"
umask 7777; echo "st $?"
umask
umask 17777; echo "st $?"
umask 66666666666666666666666666; echo "st $?"
umask 0000000000027; echo "st $?"
umask
umask 8; echo "st $?"
umask -S
umask -p
umask -p -S
