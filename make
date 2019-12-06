
mkdir -p build
cd build
gcc -o build_evict -I../include ../page_set.c ../list.c ../build_evict.c
cd ..