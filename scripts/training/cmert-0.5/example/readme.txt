mkdir example1 
../enhanced-mert -d 8
mv cmert.log weights.txt reduced_* example1

mkdir example2 
../enhanced-mert -d 8 -activate 2,4,7,8
mv cmert.log weights.txt reduced_* example2

mkdir example3
../enhanced-mert -d 8 -activate 1,5
mv cmert.log weights.txt reduced_* example3

