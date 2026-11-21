# With cdable_vars, cd prints the value it took after changing
# directory, which may have replaced that very variable (OLDPWD, PWD)
shopt -s cdable_vars
mkdir -p dir-with-a-long-enough-name/sub
cd dir-with-a-long-enough-name/sub
OLDPWD=$HOME/./dir-with-a-long-enough-name/../dir-with-a-long-enough-name
cd OLDPWD | sed "s|$HOME|HOME|"
cd OLDPWD; pwd | sed "s|$HOME|HOME|"; echo "${OLDPWD/#$HOME/HOME}"
PWD=$HOME/./dir-with-a-long-enough-name/sub/.
cd /
cd PWD | sed "s|$HOME|HOME|"
