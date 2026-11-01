# while read ... done <file runs in the current shell
printf '%s\n' alpha beta gamma >/tmp/f
count=0
while read line; do
    count=$((count+1))
    last=$line
done </tmp/f
echo "count=$count last=$last"
while read a; do echo "outer $a"; read b; echo "inner $b"; done </tmp/f
read first </tmp/f
read again </tmp/f
echo "$first $again"
while read x; do echo "never"; done </tmp/nosuch
echo status $?
