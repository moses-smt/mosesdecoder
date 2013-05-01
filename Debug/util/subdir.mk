################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../util/getopt.c 

CC_SRCS += \
../util/bit_packing.cc \
../util/bit_packing_test.cc \
../util/ersatz_progress.cc \
../util/exception.cc \
../util/file.cc \
../util/file_piece.cc \
../util/file_piece_test.cc \
../util/joint_sort_test.cc \
../util/mmap.cc \
../util/multi_intersection_test.cc \
../util/murmur_hash.cc \
../util/pool.cc \
../util/probing_hash_table_test.cc \
../util/read_compressed.cc \
../util/read_compressed_test.cc \
../util/scoped.cc \
../util/sorted_uniform_test.cc \
../util/string_piece.cc \
../util/tokenize_piece_test.cc \
../util/usage.cc 

OBJS += \
./util/bit_packing.o \
./util/bit_packing_test.o \
./util/ersatz_progress.o \
./util/exception.o \
./util/file.o \
./util/file_piece.o \
./util/file_piece_test.o \
./util/getopt.o \
./util/joint_sort_test.o \
./util/mmap.o \
./util/multi_intersection_test.o \
./util/murmur_hash.o \
./util/pool.o \
./util/probing_hash_table_test.o \
./util/read_compressed.o \
./util/read_compressed_test.o \
./util/scoped.o \
./util/sorted_uniform_test.o \
./util/string_piece.o \
./util/tokenize_piece_test.o \
./util/usage.o 

C_DEPS += \
./util/getopt.d 

CC_DEPS += \
./util/bit_packing.d \
./util/bit_packing_test.d \
./util/ersatz_progress.d \
./util/exception.d \
./util/file.d \
./util/file_piece.d \
./util/file_piece_test.d \
./util/joint_sort_test.d \
./util/mmap.d \
./util/multi_intersection_test.d \
./util/murmur_hash.d \
./util/pool.d \
./util/probing_hash_table_test.d \
./util/read_compressed.d \
./util/read_compressed_test.d \
./util/scoped.d \
./util/sorted_uniform_test.d \
./util/string_piece.d \
./util/tokenize_piece_test.d \
./util/usage.d 


# Each subdirectory must supply rules for building sources it contributes
util/%.o: ../util/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

util/%.o: ../util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


