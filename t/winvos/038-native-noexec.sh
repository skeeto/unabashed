# personality: native
# Hand-checked baseline against busybox-w32 sh: a file that native
# mode does not count as executable (no extension, no #!) is refused,
# by path or found on PATH; a .sh file without #! runs as shell code.
mkdir w && cd w
echo 'echo plain ran' >plain
echo 'echo sh ran' >x.sh
./plain
echo "status $?"
mkdir d && cp plain d/plain2
PATH="$PWD/d;$PATH" plain2
echo "status $?"
./x.sh
echo "status $?"
