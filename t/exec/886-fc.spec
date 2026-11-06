== stdout 767
aa ab ac
two
1	 echo aa ab ac
2	 echo two
	 echo aa ab ac
	 echo two
4	 fc -nl 1 2
3	 fc -l
6	 fc -l -0
2	 echo two
3	 fc -l
4	 fc -nl 1 2
5	 fc -lr -2
6	 fc -l -0
fcs: line 9: fc: no command found
st 1
echo two
two
st 0
fcs: line 11: fc: no command found
fcs: line 12: fc: no command found
st 1
fcs: line 13: fc: history specification out of range
st 1
fcs: line 14: fc: -v: invalid option
fc: usage: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
st 2
fcs: line 15: fc: -e: option requires an argument
fc: usage: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
st 2
(exit 42)
st 42
st 1
edited
echo edited
echo edited
edited
echo edited
echo edited
edited
   17  echo edited
   18  echo edited
   19  echo edited
   20  history 4

== stderr 0

== merged 767
aa ab ac
two
1	 echo aa ab ac
2	 echo two
	 echo aa ab ac
	 echo two
4	 fc -nl 1 2
3	 fc -l
6	 fc -l -0
2	 echo two
3	 fc -l
4	 fc -nl 1 2
5	 fc -lr -2
6	 fc -l -0
fcs: line 9: fc: no command found
st 1
echo two
two
st 0
fcs: line 11: fc: no command found
fcs: line 12: fc: no command found
st 1
fcs: line 13: fc: history specification out of range
st 1
fcs: line 14: fc: -v: invalid option
fc: usage: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
st 2
fcs: line 15: fc: -e: option requires an argument
fc: usage: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
st 2
(exit 42)
st 42
st 1
edited
echo edited
echo edited
edited
echo edited
echo edited
edited
   17  echo edited
   18  echo edited
   19  echo edited
   20  history 4

== status 0
