# The time keyword reports per TIMEFORMAT on standard error
TIMEFORMAT='timed %% %Q'
time echo a
time ! true; echo $?
! time true; echo $?
time time echo b
TIMEFORMAT='timed %%'
time echo c
time ! true; echo $?
! time true; echo $?
time time echo d
{ time; echo after; } 2>&1 | wc -l
TIMEFORMAT=
time echo quiet
unset TIMEFORMAT
{ time -p true; } 2>&1 | wc -l
{ time true; } 2>&1 | wc -l
{ time -- true; } 2>&1 | wc -l
