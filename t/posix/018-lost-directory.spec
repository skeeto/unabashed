== stdout 2723
## moved-pwdP
W/e/sub
st 0
W/e/sub
PWD W/d/sub
W/e/sub
W/e/sub
## moved-posix
W/e/sub
st 0 PWD W/d/sub
W/e/sub
PWD W/e/sub
## moved-cdP
st 0 PWD W/e/sub OLDPWD W/d/sub
W/e/sub
## moved-cdP-dotdot
st 0 PWD W/e
W/e
## moved-cd
st 0 PWD W/e/sub
W/e/sub
## moved-posix-cd
bash: line 1: cd: .: No such file or directory
st 1
st 0 PWD W/e/sub
## moved-cd-sub
st 0 PWD W/e/sub/x
PWD W/e
## moved-cd-missing
bash: line 1: cd: nothere: No such file or directory
st 1 PWD W/d/sub
## moved-cdpath
W/e/sub/x
st 0 PWD W/e/sub/x
## moved-link
W/e/sub
st 0
W/e/sub
st 0 PWD W/e/sub
## relinked
W/b/s
W/l/s
W/a/s
W/a/s
PWD W/a/s
## removed-pwdP
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1 PWD W/d
## removed-posix
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1 PWD W/d
## removed-cdP
cd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD W/d/.
W/d/.
## removed-cd
cd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD W/d/./.
W/d/./.
## removed-setP
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0
## removed-dotdot
st 0 PWD W
W
## lost-dot
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD .
.
.
st 0
st 0 PWD . OLDPWD .
.
## lost-name
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
bash: line 1: cd: x: No such file or directory
st 1
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0
bash: line 1: cd: W/d: No such file or directory
st 1
## dotdot
bash: line 1: cd: nonexist/..: No such file or directory
st 1
bash: line 1: cd: /nonexist/../tmp: No such file or directory
st 1
bash: line 1: cd: f/..: Not a directory
st 1
st 0
bash: line 1: cd: ../nonexist/..: No such file or directory
st 1
bash: line 1: cd: ../nonexist/..: No such file or directory
st 1 PWD W/a
## dotdot-link
st 0 PWD W/l
bash: line 1: cd: ../l/sub/..: No such file or directory
st 1

== stderr 0

== merged 2723
## moved-pwdP
W/e/sub
st 0
W/e/sub
PWD W/d/sub
W/e/sub
W/e/sub
## moved-posix
W/e/sub
st 0 PWD W/d/sub
W/e/sub
PWD W/e/sub
## moved-cdP
st 0 PWD W/e/sub OLDPWD W/d/sub
W/e/sub
## moved-cdP-dotdot
st 0 PWD W/e
W/e
## moved-cd
st 0 PWD W/e/sub
W/e/sub
## moved-posix-cd
bash: line 1: cd: .: No such file or directory
st 1
st 0 PWD W/e/sub
## moved-cd-sub
st 0 PWD W/e/sub/x
PWD W/e
## moved-cd-missing
bash: line 1: cd: nothere: No such file or directory
st 1 PWD W/d/sub
## moved-cdpath
W/e/sub/x
st 0 PWD W/e/sub/x
## moved-link
W/e/sub
st 0
W/e/sub
st 0 PWD W/e/sub
## relinked
W/b/s
W/l/s
W/a/s
W/a/s
PWD W/a/s
## removed-pwdP
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1 PWD W/d
## removed-posix
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1 PWD W/d
## removed-cdP
cd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD W/d/.
W/d/.
## removed-cd
cd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD W/d/./.
W/d/./.
## removed-setP
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 1
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0
## removed-dotdot
st 0 PWD W
W
## lost-dot
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0 PWD .
.
.
st 0
st 0 PWD . OLDPWD .
.
## lost-name
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
bash: line 1: cd: x: No such file or directory
st 1
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
st 0
bash: line 1: cd: W/d: No such file or directory
st 1
## dotdot
bash: line 1: cd: nonexist/..: No such file or directory
st 1
bash: line 1: cd: /nonexist/../tmp: No such file or directory
st 1
bash: line 1: cd: f/..: Not a directory
st 1
st 0
bash: line 1: cd: ../nonexist/..: No such file or directory
st 1
bash: line 1: cd: ../nonexist/..: No such file or directory
st 1 PWD W/a
## dotdot-link
st 0 PWD W/l
bash: line 1: cd: ../l/sub/..: No such file or directory
st 1

== status 0
