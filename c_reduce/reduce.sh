# optionally preprocess
# save some backup files

godelta8
c_delta.pl ./test1.sh small.c --all
godelta8
indent small.c

./test1.sh ; echo $?
