# A trapped or ignored stop signal sent to the shell itself
trap 'echo t' TSTP; kill -TSTP $$; echo "a $?"
trap '' TTIN; kill -TTIN $$; echo "b $?"
trap 'echo u' TTOU; ( kill -TTOU $$; echo "c $?" ); echo d
trap - TSTP; trap 'echo v' TSTP; kill -s TSTP $$; echo "e $?"
