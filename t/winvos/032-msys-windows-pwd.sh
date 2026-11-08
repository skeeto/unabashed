# personality: msys
# Hand-checked baseline against Git Bash 5.3 (M9 follow-up): a PWD in
# Windows form, as a native parent such as busybox sh exports it, names
# the working directory, so bash keeps it, and cd joins relative names
# onto it in that form. Made POSIX first, a one-letter directory under
# the install root (C:/msys/t, /t) would be read as drive T:. -P gives
# the physical directory's POSIX name, as getcwd; an absolute Windows
# name is POSIX after cd, as before.
mkdir -p C:/msys/t/u
cd C:/msys
PWD=C:/msys bash -c '
echo "start $PWD"
cd .; pwd
cd t; pwd
cd u; pwd
cd ../..; pwd
cd ./t/..; pwd
cd -P t; pwd
cd C:/work; pwd
cd ..; pwd
'
