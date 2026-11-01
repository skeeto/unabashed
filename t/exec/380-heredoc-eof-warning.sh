# a here-document ended by end of input draws a warning
echo before
while IFS= read -r l; do echo "[$l]"; done <<END
body one
body $HOME
