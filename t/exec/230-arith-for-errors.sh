# arithmetic errors inside for ((...))
for ((i=0; i<2; i++)); do echo "ok $i"; done
for ((i=0; i < ; i++)); do echo "body $i"; done
echo "after syntax error: $?"
for ((i=0; i<2; i += 1/0)); do echo "step $i"; done
echo "after step error: $?"
echo end
