mkdir -p example1 
../enhanced-mert -d 8 >& cmert.log
mv cmert.log weights.txt example1

mkdir -p example2 
../enhanced-mert -d 8 -activate 2,4,7,8 >& cmert.log
mv cmert.log weights.txt reduced_* example2

mkdir -p example3
../enhanced-mert -d 8 -activate 1,3,7 >& cmert.log
mv cmert.log weights.txt reduced_* example3

