echo start
if [[ 1+ -eq 1 ]]; then echo then; else echo "else $?"; fi
echo "after $?"
[[ 1 -eq 1/0 ]]; echo "div $?"
echo end
