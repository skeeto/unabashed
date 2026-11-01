# break and return in sourced files
echo 'echo "sourced $i"; [ $i = 2 ] && break' > /tmp/b.sh
for i in 1 2 3; do . /tmp/b.sh; done
echo "after loop i=$i"
echo 'for k in 1 2 3; do [ $k = 2 ] && return 5; echo "k=$k"; done; echo never' > /tmp/r.sh
. /tmp/r.sh
echo "return from source: $?"
echo 'continue' > /tmp/c.sh
for i in 1 2; do . /tmp/c.sh; echo never; done
echo "continue via source: $i"
