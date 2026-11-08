# RANDOM and SRANDOM: properties, not values. Each subshell, comsub and
# pipeline stage starts a sequence of its own (bash reseeds them), a
# seed repeats its sequence, and values stay in range. A chance match
# of two 15-bit values (1 in 32768) would fail a "differ" line.
same() { [ "$1" = "$2" ] && echo same || echo differ; }
RANDOM=5; a=$RANDOM; b=$RANDOM
RANDOM=5; same "$a $b" "$RANDOM $RANDOM"
RANDOM=5; c=$RANDOM; RANDOM=5; same "$c" "$(echo $RANDOM)"
same "$(echo $RANDOM $RANDOM)" "$(echo $RANDOM $RANDOM)"
same "$( (RANDOM=9; echo $RANDOM) )" "$( (RANDOM=9; echo $RANDOM) )"
x=$( (echo $RANDOM $RANDOM) ); y=$( (echo $RANDOM $RANDOM) ); same "$x" "$y"
p=$(echo $RANDOM $RANDOM | cat); q=$(echo $RANDOM $RANDOM | cat); same "$p" "$q"
same "$(echo $SRANDOM)" "$(echo $SRANDOM)"
same "$SRANDOM" "$SRANDOM"
# twenty subshells: twenty different pairs
for i in $(seq 20); do (echo $RANDOM $RANDOM); done | sort -u | wc -l
bad=0
for i in $(seq 200); do
    r=$RANDOM s=$SRANDOM
    ((r<0 || r>32767 || s<0 || s>4294967295)) && bad=$((bad+1))
done
echo "out of range: $bad"
read -r u v < <(echo $RANDOM $RANDOM); same "$u $v" "$(echo $RANDOM $RANDOM)"
