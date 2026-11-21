# compgen and complete set bash's filename hooks: a directory with a $
# expansion is expanded to read it, and the matches keep it as written,
# unless direxpand. Removing a completion before any was ever defined
# succeeds silently, as bash finds no table to look in; once one was,
# a missing name is reported.
complete -r nosuch; echo "a $?"
complete -r -D; echo "b $?"
mkdir -p d/sub d/sub2; : >d/file
D=d
compgen -d '$D/' | sort; echo "c $?"
compgen -f '${D}/' | sort; echo "d $?"
compgen -d '$D/s' | sort; echo "e $?"
compgen -o default '$D/f'; echo "f $?"
compgen -f '$(echo d)/'; echo "g $?"
shopt -s direxpand
compgen -d '$D/' | sort; echo "h $?"
shopt -u direxpand
complete -d x
complete -r x nosuch; echo "i $?"
complete -r nosuch; echo "j $?"
