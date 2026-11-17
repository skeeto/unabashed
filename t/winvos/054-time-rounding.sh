# personality: native
# Hand-checked against bash 5.3's mkfmt: TIMEFORMAT's times are taken
# to the microsecond and rounded half up to their places (none:
# truncated). The vos's sleep takes exactly as long, plus 1us to start.
# Where bash would print a carry out of the fraction as a ':' digit
# (0.9996 at 3 places is 0.:00), the carry reaches the seconds.
TIMEFORMAT='%3R %1R %0R %3lR %6R %2lE'
for t in 0.0594 0.0596 0.0494 0.9996 59.9996 0.0004 0.04; do
    time sleep $t
done
