readonly ro=5
(( ro = 6 )); echo "assign status $? ro=$ro"
(( ro++ )); echo "inc status $? ro=$ro"
let ro+=1; echo "let status $? ro=$ro"
(( other = ro + 1 )); echo "other=$other"
echo end
echo "exp $(( ro = 7 ))"; echo same-line
echo "after-exp $? ro=$ro"
declare -i ri=1
readonly ri
ri=2; echo "after declare-i assign $? ri=$ri"
echo last
