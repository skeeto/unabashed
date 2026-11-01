# Signals ignored on entry are listed as ignored, and stay ignored
trap '' USR1 TERM
bash -c 'trap -p; trap "echo x" USR1; trap - TERM; trap -p; kill -USR1 $$; echo alive'
( trap -p USR1 )
