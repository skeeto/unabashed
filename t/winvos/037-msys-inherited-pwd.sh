# personality: msys
# Hand-checked baseline: a child shell keeps an inherited PWD that names
# its working directory, spelled as given, and pwd prints it (it once
# printed the freed buffer: NULs).
cd /tmp
"$BASH" -c 'pwd | od -An -c; echo "$PWD"; cd . && pwd'
