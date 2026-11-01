for i in 1 2; do echo "body $i"; break abc; echo "after $i"; done
echo "after loop st=$?"
