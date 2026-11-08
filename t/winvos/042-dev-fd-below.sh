# personality: msys
# Hand-checked baseline: a path below a device is "Not a directory",
# /dev/fd/N included (it once said "No such file or directory"), as on
# Linux.
mkdir w && cd w
echo x >/dev/fd/1/x
echo x >/dev/stdout/x
echo x >/dev/null/x
echo x >/dev/fd/1
