# an error in the init clause of for ((...))
for ((i = 1 +; i<2; i++)); do echo "body $i"; done
echo "status $?"
for ((i=0; 1/0; i++)); do echo "body $i"; done
echo "status $?"
