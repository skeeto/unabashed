# A pipeline stage, & job or coprocess started directly, that turns out
# to be shell code (no #!), runs as such, but is traced once, as bash's
# forked child traces it once before it execs
printf 'echo ns\n' > ns; chmod +x ns
{
    set -x
    ./ns | cat
    ./ns a | ./ns b | cat
    ./ns c & wait
    coproc ./ns d; wait
    set +x
} 2>log
grep -c '^+ ./ns$' log
grep -c '^+ ./ns [a-d]$' log
