# personality: msys
# Hand-checked baseline against Git Bash 5.3.15: msys mode's pwd takes
# -W, and its synopsis says so in help, help -d and the usage message.
help pwd | head -n1
help -s pwd
pwd -X
pwd --help | grep -c W
