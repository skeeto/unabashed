# printf's floating point conversions: long double through strtold
printf '[%f] [%e] [%g] [%a]\n' 3.14159 3.14159 3.14159 3.14159
printf '[%F] [%E] [%G] [%A]\n' 0.1 0.1 1e-10 0.1
printf '[%.0f|%.0f|%.0f|%.0f]\n' 0.5 1.5 2.5 3.5
printf '[%.2f|%.3f|%.20f]\n' 1.005 2.0005 0.1
printf '[%g|%g|%g|%g|%g]\n' 100000 1000000 0.0001 0.00001 123456789
printf '[%#g|%#.3g|%#.0f|%#.0e|%#.2g]\n' 1 9.9999 3 3 99.99
printf '[%010.3f|%-10.2e|%+f|% f|%+010f|%010f]\n' 3.14159 -2.5 1 1 inf nan
printf '[%*.*f|%-*.*e|%.*g]\n' 12 3 2.71828 12 2 2.71828 -1 2.71828
printf '[%f|%f|%f|%f]\n' inf -inf nan -nan
printf '[%a|%a|%a|%.0a|%.1a]\n' 1 255 0 0xf.8p0 0x1.f8p0
printf '[%f]\n' 1e4932; printf '[%.3e]\n' 1e-4940
printf '[%f|%f|%f]\n' 0x10 0X1P-2 "'A"
printf '%.1f %.1f\n' 1 2 3
printf '[%Lf|%lf|%hf]\n' 1 2 3
printf -v v '%08.3f' -1.5; echo "$v"
printf '[%f]\n'
printf '[%.-5f]\n' 1
x=$(printf '%.400f' 1e-300); echo ${#x}
