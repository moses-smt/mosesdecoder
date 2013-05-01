################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../jam-files/engine/builtins.c \
../jam-files/engine/class.c \
../jam-files/engine/command.c \
../jam-files/engine/compile.c \
../jam-files/engine/constants.c \
../jam-files/engine/debug.c \
../jam-files/engine/execnt.c \
../jam-files/engine/execunix.c \
../jam-files/engine/filent.c \
../jam-files/engine/filesys.c \
../jam-files/engine/fileunix.c \
../jam-files/engine/frames.c \
../jam-files/engine/function.c \
../jam-files/engine/glob.c \
../jam-files/engine/hash.c \
../jam-files/engine/hcache.c \
../jam-files/engine/hdrmacro.c \
../jam-files/engine/headers.c \
../jam-files/engine/jam.c \
../jam-files/engine/jambase.c \
../jam-files/engine/jamgram.c \
../jam-files/engine/lists.c \
../jam-files/engine/make.c \
../jam-files/engine/make1.c \
../jam-files/engine/md5.c \
../jam-files/engine/mem.c \
../jam-files/engine/mkjambase.c \
../jam-files/engine/modules.c \
../jam-files/engine/native.c \
../jam-files/engine/object.c \
../jam-files/engine/option.c \
../jam-files/engine/output.c \
../jam-files/engine/parse.c \
../jam-files/engine/pathunix.c \
../jam-files/engine/pwd.c \
../jam-files/engine/regexp.c \
../jam-files/engine/rules.c \
../jam-files/engine/scan.c \
../jam-files/engine/search.c \
../jam-files/engine/strings.c \
../jam-files/engine/subst.c \
../jam-files/engine/timestamp.c \
../jam-files/engine/variable.c \
../jam-files/engine/w32_getreg.c \
../jam-files/engine/yyacc.c 

OBJS += \
./jam-files/engine/builtins.o \
./jam-files/engine/class.o \
./jam-files/engine/command.o \
./jam-files/engine/compile.o \
./jam-files/engine/constants.o \
./jam-files/engine/debug.o \
./jam-files/engine/execnt.o \
./jam-files/engine/execunix.o \
./jam-files/engine/filent.o \
./jam-files/engine/filesys.o \
./jam-files/engine/fileunix.o \
./jam-files/engine/frames.o \
./jam-files/engine/function.o \
./jam-files/engine/glob.o \
./jam-files/engine/hash.o \
./jam-files/engine/hcache.o \
./jam-files/engine/hdrmacro.o \
./jam-files/engine/headers.o \
./jam-files/engine/jam.o \
./jam-files/engine/jambase.o \
./jam-files/engine/jamgram.o \
./jam-files/engine/lists.o \
./jam-files/engine/make.o \
./jam-files/engine/make1.o \
./jam-files/engine/md5.o \
./jam-files/engine/mem.o \
./jam-files/engine/mkjambase.o \
./jam-files/engine/modules.o \
./jam-files/engine/native.o \
./jam-files/engine/object.o \
./jam-files/engine/option.o \
./jam-files/engine/output.o \
./jam-files/engine/parse.o \
./jam-files/engine/pathunix.o \
./jam-files/engine/pwd.o \
./jam-files/engine/regexp.o \
./jam-files/engine/rules.o \
./jam-files/engine/scan.o \
./jam-files/engine/search.o \
./jam-files/engine/strings.o \
./jam-files/engine/subst.o \
./jam-files/engine/timestamp.o \
./jam-files/engine/variable.o \
./jam-files/engine/w32_getreg.o \
./jam-files/engine/yyacc.o 

C_DEPS += \
./jam-files/engine/builtins.d \
./jam-files/engine/class.d \
./jam-files/engine/command.d \
./jam-files/engine/compile.d \
./jam-files/engine/constants.d \
./jam-files/engine/debug.d \
./jam-files/engine/execnt.d \
./jam-files/engine/execunix.d \
./jam-files/engine/filent.d \
./jam-files/engine/filesys.d \
./jam-files/engine/fileunix.d \
./jam-files/engine/frames.d \
./jam-files/engine/function.d \
./jam-files/engine/glob.d \
./jam-files/engine/hash.d \
./jam-files/engine/hcache.d \
./jam-files/engine/hdrmacro.d \
./jam-files/engine/headers.d \
./jam-files/engine/jam.d \
./jam-files/engine/jambase.d \
./jam-files/engine/jamgram.d \
./jam-files/engine/lists.d \
./jam-files/engine/make.d \
./jam-files/engine/make1.d \
./jam-files/engine/md5.d \
./jam-files/engine/mem.d \
./jam-files/engine/mkjambase.d \
./jam-files/engine/modules.d \
./jam-files/engine/native.d \
./jam-files/engine/object.d \
./jam-files/engine/option.d \
./jam-files/engine/output.d \
./jam-files/engine/parse.d \
./jam-files/engine/pathunix.d \
./jam-files/engine/pwd.d \
./jam-files/engine/regexp.d \
./jam-files/engine/rules.d \
./jam-files/engine/scan.d \
./jam-files/engine/search.d \
./jam-files/engine/strings.d \
./jam-files/engine/subst.d \
./jam-files/engine/timestamp.d \
./jam-files/engine/variable.d \
./jam-files/engine/w32_getreg.d \
./jam-files/engine/yyacc.d 


# Each subdirectory must supply rules for building sources it contributes
jam-files/engine/%.o: ../jam-files/engine/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


