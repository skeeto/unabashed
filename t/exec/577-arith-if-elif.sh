grade() {
  if (( $1 >= 90 )); then echo A
  elif (( $1 >= 80 )); then echo B
  elif [[ $1 -ge 70 ]]; then echo C
  elif [ "$1" -ge 60 ]; then echo D
  else echo F
  fi
}
for s in 95 85 75 65 5 0x5A 077; do printf '%s -> ' "$s"; grade "$s"; done
