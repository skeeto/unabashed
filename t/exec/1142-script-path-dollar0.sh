# A script without #! that the shell runs itself has as $0 the path it
# tried to execute, as bash's execve fallback: the PATH search's, and
# for exec the full path (full_pathname); exec -a names it
mkdir d; printf 'echo "${0/#$PWD/PWD}"\n' > d/sc; chmod +x d/sc
printf 'echo x\n' > d/un; chmod 111 d/un
PATH=$PWD/d:$PATH
sc
(exec sc)
(exec -a foo sc)
d/sc
./d/sc
(exec ./d/sc)
(cd d && exec ./sc)
(exec d/un) 2>&1 | sed "s|$PWD|PWD|"
