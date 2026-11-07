# personality: native
# Hand-checked baseline: busybox-w32 1.39.0 gives the same on a real
# C:/Windows. In native mode cd takes each directory's case from the
# disk, and the drive letter upper case, whatever was typed
cd c:/windows/SYSTEM32; echo "$PWD"; pwd
cd ../../wINDOWS/./system32/..; echo "$PWD"
cd -P c:/WINDOWS; echo "$PWD"
cd c:/; cd windows/sYSTEM32; echo "$PWD"
