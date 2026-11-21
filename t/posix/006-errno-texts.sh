# Errors from the system, in Linux's numbering whatever the host (the
# POSIX layer translates macOS's, which numbers ELOOP, ENAMETOOLONG
# and others otherwise): each with its text, none "Unknown error"
ln -s loop2 loop1; ln -s loop1 loop2
long=$(printf '%0256d' 0)
cat < loop1
echo x > loop1
echo x > "$long"
. ./loop1
cd loop1
cd "$long"
./loop1; echo "status $?"
"./$long"; echo "status $?"
