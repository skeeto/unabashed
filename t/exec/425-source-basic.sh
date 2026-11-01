# source and . of a file created by a heredoc
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/s1.sh <<'END'
echo in sourced
sv=sourced-var
sf() { echo sourced-func; }
END
source /tmp/s1.sh
echo "$sv"
sf
. /tmp/s1.sh
echo "status $?"
cd /tmp
. ./s1.sh
source s1.sh
echo "final $?"
