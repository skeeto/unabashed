# hash -p refuses a directory, and names with a slash are skipped
hash -r
hash -p / root; echo "status $?"
hash -p /opt/fakes/bin/cat kitty; echo "status $?"
hash -p / a b; echo "status $?"
hash -p /opt/fakes/bin/cat ./x; echo "status $?"
hash -l
hash ./x; echo "status $?"
hash -d ./x; echo "status $?"
hash -t ./x; echo "status $?"
hash -d /bin/x; echo "status $?"
