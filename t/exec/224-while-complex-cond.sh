# while conditions that are lists: the last status decides
i=0
while echo "test $i"; [ $i -lt 2 ]; do
    i=$((i+1))
done
echo "--"
i=0
while [ $i -lt 5 ] && [ $i != 3 ]; do echo "and $i"; i=$((i+1)); done
i=0
while [ $i = 9 ] || [ $i -lt 2 ]; do echo "or $i"; i=$((i+1)); done
i=0
while ! [ $i -ge 2 ]; do echo "not $i"; i=$((i+1)); done
i=0
while { i=$((i+1)); [ $i -lt 3 ]; }; do echo "group $i"; done
i=0
while (exit $i); do echo "sub $i"; i=1; done
i=0
while if [ $i -lt 2 ]; then true; else false; fi; do echo "if $i"; i=$((i+1)); done
i=0
while case $i in 0|1) true;; *) false;; esac; do echo "case $i"; i=$((i+1)); done
echo "end $?"
