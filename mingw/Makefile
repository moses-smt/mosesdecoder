	
# Environment
MKDIR=C:\MinGW\msys\1.0\bin\mkdir.exe
CP=C:\MinGW\msys\1.0\bin\cp.exe
GREP=C:\MinGW\msys\1.0\bin\grep.exe
SED=C:\MinGW\msys\1.0\bin\sed.exe
RM=C:\MinGW\msys\1.0\bin\rm.exe
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as
AR=ar

# Macros
CND_PLATFORM=MinGW-Windows
CND_DLIB_EXT=dll
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Object Directory
OBJECTDIR=${CND_BUILDDIR}\${CND_CONF}\${CND_PLATFORM}

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=-g -Wall -DKENLM_MAX_ORDER=6 -DMAX_NUM_FACTORS=4 -D_WIN32 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I. -ID:\work\moses\dependencies\boost_1_54_0_install\include
# -DWITH_THREADS

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-LD:\work\moses\dependencies\boost_1_54_0_install\lib

.PHONY : all
all: ${OBJECTDIR}\lmplz.exe ${OBJECTDIR}\moses-cmd.exe ${OBJECTDIR}\build_binary.exe \
	${OBJECTDIR}\moses-chart-cmd.exe ${OBJECTDIR}\processPhraseTable.exe \
	${OBJECTDIR}\processLexicalTable.exe ${OBJECTDIR}\CreateOnDiskPt.exe
	
${OBJECTDIR}\\%.d: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	- ${RM} -f $@
	${CXX} -MM $(CXXFLAGS) $< > $@.temp 
	${SED} "1s/${*F}.o[ :]*/${subst \,\\\\,${OBJECTDIR}\$*}.o ${subst \,\\\\,${OBJECTDIR}\$*}.d : /g" < $@.temp > $@
	- ${RM} -f $@.temp 

${OBJECTDIR}\\%.d: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	- ${RM} -f $@
	${CXX} -MM $(CXXFLAGS) $< > $@.temp 
	${SED} "1s/${*F}.o[ :]*/${subst \,\\\\,${OBJECTDIR}\$*}.o ${subst \,\\\\,${OBJECTDIR}\$*}.d : /g" < $@.temp > $@
	- ${RM} -f $@.temp 

# libutil
UTIL_CC_SLASH = ${wildcard util/*.cc util/stream/*.cc util/double-conversion/*.cc}
UTIL_CC = ${subst /,\,${UTIL_CC_SLASH}}
UTIL_O = ${addprefix ${OBJECTDIR}\,${UTIL_CC:%.cc=%.o}}
sinclude ${UTIL_O:%.o=%.d}
	
${UTIL_O}: ${OBJECTDIR}\\%.o: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	$(CXX) $(CXXFLAGS) -c $< -o $@

${OBJECTDIR}\libutil.a: ${UTIL_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${AR} -r -s ${OBJECTDIR}\libutil.a ${UTIL_O}

# libkenlm
KENLM_CC_ALL_SLASH = ${wildcard lm/*.cc lm/builder/*.cc lm/filter/*.cc}
KENLM_CC_ALL = ${subst /,\,${KENLM_CC_ALL_SLASH}}
KENLM_CC = ${filter-out %main.cc %test.cc,${KENLM_CC_ALL}}
KENLM_O = ${addprefix ${OBJECTDIR}\,${KENLM_CC:%.cc=%.o}}
sinclude ${KENLM_O:%.o=%.d}

${KENLM_O}: ${OBJECTDIR}\\%.o: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@
	
${OBJECTDIR}\libkenlm.a: ${KENLM_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${AR} -r -s ${OBJECTDIR}\libkenlm.a ${KENLM_O}
	
#lmplz
sinclude ${OBJECTDIR}\lm\builder\lmplz_main.d

${OBJECTDIR}\lm\builder\lmplz_main.o: ${OBJECTDIR}\\%.o: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

${OBJECTDIR}\lmplz.exe: ${OBJECTDIR}\lm\builder\lmplz_main.o ${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libutil.a 
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${OBJECTDIR}\lm\builder\lmplz_main.o \
		-lkenlm -lutil \
		-lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt \
		-o ${OBJECTDIR}\lmplz.exe

#build_binary
sinclude ${OBJECTDIR}\lm\build_binary_main.d

${OBJECTDIR}\lm\build_binary_main.o: ${OBJECTDIR}\\%.o: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

${OBJECTDIR}\build_binary.exe: ${OBJECTDIR}\lm\build_binary_main.o ${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libutil.a
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${OBJECTDIR}\lm\build_binary_main.o \
		-lkenlm -lutil \
		-lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt \
		-o ${OBJECTDIR}\build_binary.exe 

#libondiskpt
ONDISKPT_CC = ${addprefix OnDiskPt\, OnDiskWrapper.cpp SourcePhrase.cpp TargetPhrase.cpp Word.cpp Phrase.cpp PhraseNode.cpp TargetPhraseCollection.cpp Vocab.cpp OnDiskQuery.cpp}
ONDISKPT_O = ${addprefix ${OBJECTDIR}\,${ONDISKPT_CC:%.cpp=%.o}}
sinclude ${ONDISKPT_O:%.o=%.d}
	
${ONDISKPT_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

${OBJECTDIR}\libondiskpt.a: ${ONDISKPT_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${AR} -r -s ${OBJECTDIR}\libondiskpt.a ${ONDISKPT_O}
	
#libmoses
MOSES_CC_ALL_SLASH = ${wildcard moses/*.cpp moses/TranslationModel/*.cpp \
			moses/TranslationModel/fuzzy-match/*.cpp \
			moses/TranslationModel/DynSAInclude/*.cpp \
			moses/TranslationModel/RuleTable/*.cpp \
			moses/TranslationModel/Scope3Parser/*.cpp \
			moses/TranslationModel/CYKPlusParser/*.cpp \
			moses/FF/*.cpp \
			moses/FF/OSM-Feature/*.cpp \
			moses/FF/LexicalReordering/*.cpp\
			moses/TranslationModel/CompactPT/*.cpp}
#lm
MOSES_CC_ALL_SLASH += ${addprefix moses\LM\, Backward.cpp BackwardLMState.cpp Base.cpp Implementation.cpp Joint.cpp Ken.cpp MultiFactor.cpp SingleFactor.cpp SkeletonLM.cpp}
MOSES_CC_ALL = ${subst /,\,${MOSES_CC_ALL_SLASH}}
MOSES_CC = ${filter-out %Test.cpp moses\Mock%.cpp,${MOSES_CC_ALL}}
MOSES_O = ${addprefix ${OBJECTDIR}\,${MOSES_CC:%.cpp=%.o}}
sinclude ${MOSES_O:%.o=%.d}
	
${MOSES_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

SEARCH_CC = ${addprefix search\, edge_generator.cc nbest.cc rule.cc vertex.cc}
SEARCH_O = ${addprefix ${OBJECTDIR}\,${SEARCH_CC:%.cc=%.o}}
sinclude ${SEARCH_O:%.o=%.d}
	
${SEARCH_O}: ${OBJECTDIR}\\%.o: %.cc
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

${OBJECTDIR}\libmoses.a: ${MOSES_O} ${SEARCH_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${AR} -r -s ${OBJECTDIR}\libmoses.a ${MOSES_O} ${SEARCH_O}
	
#libmosescmd
MOSESCMD_CC = ${addprefix moses-cmd\, IOWrapper.cpp mbr.cpp LatticeMBR.cpp TranslationAnalysis.cpp}
MOSESCMD_O = ${addprefix ${OBJECTDIR}\,${MOSESCMD_CC:%.cpp=%.o}}
sinclude ${MOSESCMD_O:%.o=%.d}
	
${MOSESCMD_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

#moses-cmd
sinclude ${OBJECTDIR}\moses-cmd\Main.d

${OBJECTDIR}\moses-cmd\Main.o: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@
	
${OBJECTDIR}\moses-cmd.exe: ${OBJECTDIR}\libmoses.a \
				${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libondiskpt.a \
				${OBJECTDIR}\libutil.a ${OBJECTDIR}\moses-cmd\Main.o ${MOSESCMD_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${OBJECTDIR}\moses-cmd\Main.o ${MOSESCMD_O} \
		-lmoses	-lkenlm -londiskpt -lutil \
		-lboost_iostreams-mt -lboost_bzip2-mt -lboost_zlib-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt \
		-lz -lbz2 -ldl -lmman \
		-o ${OBJECTDIR}\moses-cmd.exe 

#moseschartcmd
MOSESCHARTCMD_CC = ${addprefix moses-chart-cmd\, Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp}
MOSESCHARTCMD_O = ${addprefix ${OBJECTDIR}\,${MOSESCHARTCMD_CC:%.cpp=%.o}}
sinclude ${MOSESCHARTCMD_O:%.o=%.d}
	
${MOSESCHARTCMD_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@
	
${OBJECTDIR}\moses-chart-cmd.exe: ${OBJECTDIR}\libmoses.a \
				${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libondiskpt.a \
				${OBJECTDIR}\libutil.a ${MOSESCHARTCMD_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${MOSESCHARTCMD_O} \
		-lmoses	-lkenlm -londiskpt -lutil \
		-lboost_iostreams-mt -lboost_bzip2-mt -lboost_zlib-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt \
		-lz -lbz2 -ldl -lmman \
		-o ${OBJECTDIR}\moses-chart-cmd.exe 
	
#processPhraseTable
PROCESSPHRASETABLE_CC = ${addprefix misc\, GenerateTuples.cpp  processPhraseTable.cpp}
PROCESSPHRASETABLE_O = ${addprefix ${OBJECTDIR}\,${PROCESSPHRASETABLE_CC:%.cpp=%.o}}
sinclude ${PROCESSPHRASETABLE_O:%.o=%.d}
	
${PROCESSPHRASETABLE_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@
	
${OBJECTDIR}\processPhraseTable.exe: ${OBJECTDIR}\libmoses.a ${OBJECTDIR}\libutil.a ${PROCESSPHRASETABLE_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${PROCESSPHRASETABLE_O} \
		-lmoses	-lutil \
		-lboost_iostreams-mt -lboost_bzip2-mt -lboost_zlib-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt \
		-lz -lbz2 -ldl -lmman \
		-o ${OBJECTDIR}\processPhraseTable.exe 
	
#processLexicalTable
PROCESSLEXTABLE_CC = ${addprefix misc\, processLexicalTable.cpp}
PROCESSLEXTABLE_O = ${addprefix ${OBJECTDIR}\,${PROCESSLEXTABLE_CC:%.cpp=%.o}}
sinclude ${PROCESSLEXTABLE_O:%.o=%.d}
	
${PROCESSLEXTABLE_O}: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@
	
${OBJECTDIR}\processLexicalTable.exe: ${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libmoses.a ${OBJECTDIR}\libutil.a ${OBJECTDIR}\libondiskpt.a ${PROCESSLEXTABLE_O}
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${PROCESSLEXTABLE_O} \
		-lmoses	-lkenlm -londiskpt -lutil \
		-lboost_iostreams-mt -lboost_bzip2-mt -lboost_zlib-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt \
		-lz -lbz2 -ldl -lmman \
		-o ${OBJECTDIR}\processLexicalTable.exe 
	
#ondiskpt
sinclude ${OBJECTDIR}\OnDiskPt\Main.d

${OBJECTDIR}\OnDiskPt\Main.o: ${OBJECTDIR}\\%.o: %.cpp
	${MKDIR} -p ${subst \,/,$(@D)}
	${CXX} ${CXXFLAGS} -c $< -o $@

${OBJECTDIR}\CreateOnDiskPt.exe: ${OBJECTDIR}\OnDiskPt\Main.o ${OBJECTDIR}\libondiskpt.a ${OBJECTDIR}\libmoses.a ${OBJECTDIR}\libkenlm.a ${OBJECTDIR}\libutil.a
	${MKDIR} -p ${subst \,/,${OBJECTDIR}}
	${CXX} ${LDLIBSOPTIONS} -L ${OBJECTDIR} -static ${OBJECTDIR}\OnDiskPt\Main.o \
		-londiskpt -lmoses -lkenlm -lutil \
		-lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt \
		-lz -lbz2 -ldl -lmman \
		-o ${OBJECTDIR}\CreateOnDiskPt.exe 

.PHONY : clean
clean: 

