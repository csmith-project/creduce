echo > delta.log

date >> delta.log

${CSMITH_HOME}/utah/scripts/reduce.sh >> delta.log

echo here1
grep -c here1 delta.log >> delta.log
echo here2
grep -c here2 delta.log >> delta.log
echo here3
grep -c here3 delta.log >> delta.log
echo here4
grep -c here4 delta.log >> delta.log
echo here5
grep -c here5 delta.log >> delta.log
echo here6
grep -c here6 delta.log >> delta.log
echo here7
grep -c here7 delta.log >> delta.log

date >> delta.log
