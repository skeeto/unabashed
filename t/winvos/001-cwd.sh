# personality: native
# Hand-checked baseline of today's minimal Windows layer, not recorded
# from an oracle (M5a step 4); step 5's path modes change it on purpose.
# The working directory and cd with drive paths in every spelling.
pwd
echo "$PWD"
cd /c/Windows && pwd
cd C:/Windows/System32 && pwd
cd 'C:\Windows' && pwd
cd c:/WORK && pwd
cd D: && pwd
cd E:/
echo "status $?"
cd .. && pwd
cd C:/work/nosuch
echo "status $?"
cd C:/work && mkdir sub && cd sub && pwd && cd .. && pwd
