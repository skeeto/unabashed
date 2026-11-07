== stdout 1342
~
 0  ~
~
~
~
status 0
status 1
status 1
status 1
~/a ~
status 0
/home/oracle/a
~/b ~/a ~
/home/oracle/b
~/d e ~/b ~/a ~
/home/oracle/d e
~/d e ~/b ~/a ~
/home/oracle/d e /home/oracle/b /home/oracle/a /home/oracle
 0  ~/d e
 1  ~/b
 2  ~/a
 3  ~
~/d e
~/b
~/a
~
 0  ~/d e
 1  ~/b
 2  ~/a
 3  ~
~/d e
~/b
~
~/a
~
~/d e
status 1
status 1
status 2
status 2
status 2
~/d e ~/b ~/a ~
~/a
/home/oracle/d e /home/oracle/b /home/oracle/a /home/oracle 4
/home/oracle/d e /home/oracle/b /home/oracle/b /home/oracle /home/oracle/a /home/oracle ~4 ~+x
~/b ~/d e ~/a ~
/home/oracle/b
~/a ~ ~/b ~/d e
/home/oracle/a
~/b ~/d e ~/a ~
/home/oracle/b
~/b ~/d e ~/a ~
/home/oracle/b
~ ~/b ~/d e ~/a
/home/oracle
status 1
status 1
status 2
status 2
status 1
~ ~/b ~/d e ~/a
status 1
~ ~/c ~/b ~/d e ~/a
status 0
/home/oracle
status 0
~ ~/b ~/d e ~/a ~
/home/oracle
status 0
~/c ~ ~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
~ ~ ~/b ~/d e ~/a ~
~ /elsewhere ~/b ~/d e ~/a ~
~ /elsewhere ~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
status 0
/home/oracle
~ ~/d e ~/a ~
~ ~/d e ~
status 0
status 1
status 1
status 2
status 2
status 2
~/d e ~
/home/oracle/d e
~
status 1
status 1
status 1
status 1
~
~/b
/home/oracle/b
/home/oracle/b
~/b ~
/home/oracle/b
~/a ~ ~
~/a ~ ~
/home/oracle/a /home/oracle /home/oracle
/home/oracle/a /home/oracle /home/oracle

== stderr 1366
bash: line 6: pushd: no other directory
bash: line 7: popd: directory stack empty
bash: line 8: dirs: directory stack empty
bash: line 13: dirs: 4: directory stack index out of range
bash: line 14: dirs: 4: directory stack index out of range
bash: line 15: dirs: +x: invalid number
dirs: usage: dirs [-clpv] [+N] [-N]
bash: line 16: dirs: -lv: invalid number
dirs: usage: dirs [-clpv] [+N] [-N]
bash: line 17: dirs: foo: invalid option
dirs: usage: dirs [-clpv] [+N] [-N]
bash: line 26: pushd: +9: directory stack index out of range
bash: line 27: pushd: -9: directory stack index out of range
bash: line 28: pushd: +q: invalid number
pushd: usage: pushd [-n] [+N | -N | dir]
bash: line 29: pushd: -z: invalid number
pushd: usage: pushd [-n] [+N | -N | dir]
bash: line 30: pushd: nosuch: No such file or directory
bash: line 31: pushd: too many arguments
bash: line 42: popd: +7: directory stack index out of range
bash: line 43: popd: -7: directory stack index out of range
bash: line 44: popd: x: invalid argument
popd: usage: popd [-n] [+N | -N]
bash: line 45: popd: -: invalid number
popd: usage: popd [-n] [+N | -N]
bash: line 46: popd: +a: invalid number
popd: usage: popd [-n] [+N | -N]
bash: line 48: popd: directory stack empty
bash: line 49: popd: directory stack empty
bash: line 50: popd: directory stack empty
bash: line 51: popd: directory stack empty

== merged 2708
~
 0  ~
~
~
~
status 0
bash: line 6: pushd: no other directory
status 1
bash: line 7: popd: directory stack empty
status 1
bash: line 8: dirs: directory stack empty
status 1
~/a ~
status 0
/home/oracle/a
~/b ~/a ~
/home/oracle/b
~/d e ~/b ~/a ~
/home/oracle/d e
~/d e ~/b ~/a ~
/home/oracle/d e /home/oracle/b /home/oracle/a /home/oracle
 0  ~/d e
 1  ~/b
 2  ~/a
 3  ~
~/d e
~/b
~/a
~
 0  ~/d e
 1  ~/b
 2  ~/a
 3  ~
~/d e
~/b
~
~/a
~
~/d e
bash: line 13: dirs: 4: directory stack index out of range
status 1
bash: line 14: dirs: 4: directory stack index out of range
status 1
bash: line 15: dirs: +x: invalid number
dirs: usage: dirs [-clpv] [+N] [-N]
status 2
bash: line 16: dirs: -lv: invalid number
dirs: usage: dirs [-clpv] [+N] [-N]
status 2
bash: line 17: dirs: foo: invalid option
dirs: usage: dirs [-clpv] [+N] [-N]
status 2
~/d e ~/b ~/a ~
~/a
/home/oracle/d e /home/oracle/b /home/oracle/a /home/oracle 4
/home/oracle/d e /home/oracle/b /home/oracle/b /home/oracle /home/oracle/a /home/oracle ~4 ~+x
~/b ~/d e ~/a ~
/home/oracle/b
~/a ~ ~/b ~/d e
/home/oracle/a
~/b ~/d e ~/a ~
/home/oracle/b
~/b ~/d e ~/a ~
/home/oracle/b
~ ~/b ~/d e ~/a
/home/oracle
bash: line 26: pushd: +9: directory stack index out of range
status 1
bash: line 27: pushd: -9: directory stack index out of range
status 1
bash: line 28: pushd: +q: invalid number
pushd: usage: pushd [-n] [+N | -N | dir]
status 2
bash: line 29: pushd: -z: invalid number
pushd: usage: pushd [-n] [+N | -N | dir]
status 2
bash: line 30: pushd: nosuch: No such file or directory
status 1
~ ~/b ~/d e ~/a
bash: line 31: pushd: too many arguments
status 1
~ ~/c ~/b ~/d e ~/a
status 0
/home/oracle
status 0
~ ~/b ~/d e ~/a ~
/home/oracle
status 0
~/c ~ ~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
~ ~ ~/b ~/d e ~/a ~
~ /elsewhere ~/b ~/d e ~/a ~
~ /elsewhere ~/b ~/d e ~/a ~
~ ~/b ~/d e ~/a ~
status 0
/home/oracle
~ ~/d e ~/a ~
~ ~/d e ~
status 0
bash: line 42: popd: +7: directory stack index out of range
status 1
bash: line 43: popd: -7: directory stack index out of range
status 1
bash: line 44: popd: x: invalid argument
popd: usage: popd [-n] [+N | -N]
status 2
bash: line 45: popd: -: invalid number
popd: usage: popd [-n] [+N | -N]
status 2
bash: line 46: popd: +a: invalid number
popd: usage: popd [-n] [+N | -N]
status 2
~/d e ~
/home/oracle/d e
~
bash: line 48: popd: directory stack empty
status 1
bash: line 49: popd: directory stack empty
status 1
bash: line 50: popd: directory stack empty
status 1
bash: line 51: popd: directory stack empty
status 1
~
~/b
/home/oracle/b
/home/oracle/b
~/b ~
/home/oracle/b
~/a ~ ~
~/a ~ ~
/home/oracle/a /home/oracle /home/oracle
/home/oracle/a /home/oracle /home/oracle

== status 0
