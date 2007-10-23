mkdir -p example1 
../enhanced-mert -d 8 >& cmert.log
mv cmert.log weights.txt example1

mkdir -p example2 
../enhanced-mert -d 8 -activate lm,tm_2,tm_5,w >& cmert.log
mv cmert.log weights.txt reduced_* example2

mkdir -p example3
../enhanced-mert -d 8 -activate d,tm_1,tm_5 >& cmert.log
mv cmert.log weights.txt reduced_* example3

