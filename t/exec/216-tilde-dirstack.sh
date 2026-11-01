# ~N, ~+N and ~-N refer to the directory stack; with no pushd the
# stack holds only the current directory.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
show ~0 ~+0 ~-0
show ~1 ~+1 ~-1
show ~0/x ~00 ~+00
PWD=/elsewhere
show ~0 ~+
show ~-
cd /home/oracle
show ~- ~+ ~0
