# break and continue across many levels
for a in 1 2; do
  for b in 1 2; do
    for c in 1 2; do
      for d in 1 2; do
        for e in 1 2; do
          echo "$a$b$c$d$e"
          [ $e = 2 ] && continue 4
          [ $a = 2 ] && break 5
        done
        echo never-d
      done
    done
  done
done
echo "end $a$b$c$d$e"
for a in 1 2; do
  for b in 1 2; do
    for c in 1 2; do
      echo "$a$b$c"
      continue 3
    done
  done
done
