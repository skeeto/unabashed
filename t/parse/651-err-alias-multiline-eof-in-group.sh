shopt -s expand_aliases
alias g='{
echo x
if true; then'
echo a
g
