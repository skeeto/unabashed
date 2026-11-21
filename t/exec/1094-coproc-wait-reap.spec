== stdout 1574
-- wait pid, same line
bash: line 9: declare: COPROC: not found
COPROC_PID 
status 0
-- wait %%, same line
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- waited A
bash: line 9: declare: A: not found
A_PID 
-- B lives
declare -a B=([0]="62" [1]="58")
B_PID set
-- B input closed
declare -a B=([0]="62" [1]="-1")
B_PID set
-- waited B
bash: line 9: declare: B: not found
B_PID 
-- both closed
declare -a COPROC=([0]="-1" [1]="-1")
COPROC_PID set
-- both closed, waited
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after waiting another job
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- in function after wait
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- both waited, C
bash: line 9: declare: C: not found
C_PID 
-- both waited, D
bash: line 9: declare: D: not found
D_PID 
D open
-- dead, not noticed
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- in eval
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- subshell
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- after subshell
bash: line 9: declare: COPROC: not found
COPROC_PID 
D open
-- after comsub
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after funsub
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after pipeline
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after empty source
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- in trap
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after another
bash: line 9: declare: A: not found
A_PID 
-- the other
declare -a B=([0]="61" [1]="56")
B_PID set
D read hi

== stderr 0

== merged 1574
-- wait pid, same line
bash: line 9: declare: COPROC: not found
COPROC_PID 
status 0
-- wait %%, same line
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- waited A
bash: line 9: declare: A: not found
A_PID 
-- B lives
declare -a B=([0]="62" [1]="58")
B_PID set
-- B input closed
declare -a B=([0]="62" [1]="-1")
B_PID set
-- waited B
bash: line 9: declare: B: not found
B_PID 
-- both closed
declare -a COPROC=([0]="-1" [1]="-1")
COPROC_PID set
-- both closed, waited
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after waiting another job
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- in function after wait
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- both waited, C
bash: line 9: declare: C: not found
C_PID 
-- both waited, D
bash: line 9: declare: D: not found
D_PID 
D open
-- dead, not noticed
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- in eval
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- subshell
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- after subshell
bash: line 9: declare: COPROC: not found
COPROC_PID 
D open
-- after comsub
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after funsub
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after pipeline
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after empty source
declare -a COPROC=([0]="62" [1]="58")
COPROC_PID set
-- in trap
bash: line 9: declare: COPROC: not found
COPROC_PID 
-- after another
bash: line 9: declare: A: not found
A_PID 
-- the other
declare -a B=([0]="61" [1]="56")
B_PID set
D read hi

== status 0
