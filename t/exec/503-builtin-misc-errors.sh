local x=1
echo "local st=$?"
declare -p nosuchvar
echo "declare st=$?"
cd /nonexistent
echo "cd st=$?"
cd /tmp /home
echo "cd2 st=$?"
. /nonexistent/file
echo "dot st=$?"
source
echo "source st=$?"
type nosuchcmd
echo "type st=$?"
unset nosuch_is_fine
echo "unset st=$?"
eval 'if'
echo "eval st=$?"
eval 'echo ('
echo "eval2 st=$?"
echo end
