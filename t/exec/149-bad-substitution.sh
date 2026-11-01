# malformed ${...} report bad substitution
( echo "${x;}"; echo after ); echo "status $?"
( echo "${x!}"; echo after ); echo "status $?"
( echo "${1a}"; echo after ); echo "status $?"
( echo "${x:}"; echo after ); echo "status $?"
( echo "${-x}"; echo after ); echo "status $?"
( echo "${x$}"; echo after ); echo "status $?"
( echo "${a-b}${c.d}"; echo after ); echo "status $?"
( echo ${x%%%}; echo after ); echo "status $?"
( echo "${@@}"; echo after ); echo "status $?"
echo end
