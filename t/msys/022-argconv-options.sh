# vos
# Options and assignments: conversion restarts after "-X", '=' and ','
argv -I/usr/include -L/c/lib --dir=/foo --dir=/foo:/bar foo=/bar \
     -DX=/y -o/x -o/x/y @/file -,/x a=b=/c /x=/y --a,/b
