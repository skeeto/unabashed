# -o and -O at startup: an unknown name ends the shell with status 2,
# the name taken even when it looks like an option; without a name
# they list the options and the shell goes on
bash -O nosuch -c 'echo no'; echo "st $?"
bash -O -s -c 'echo no'; echo "st $?"
bash -o nosuch -c 'echo no' 2>&1 | sed 's/^[^ ]* line 0: [^ ]*: /X: /'; echo "st ${PIPESTATUS[0]}"
bash -O extglob +O dotglob -o noglob -c 'shopt -p extglob dotglob; echo $-'; echo "st $?"
echo 'echo ran' | bash -O | grep -e extglob -e ran
echo 'echo ran' | bash +O | grep -e extglob -e ran
echo 'echo ran' | bash -o | grep -e noglob -e ran
echo 'echo ran' | bash +o | grep -e noglob -e ran
