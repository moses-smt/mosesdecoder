JAVA_INCLUDES=/usr/lib/jvm/java-7-openjdk-amd64/include

JAVA_SOURCES=example.java src/queryPhraseTableMin.java src/queryPhraseTableMinJNI.java src/QueryPhraseTableMin.java
JAVA_CLASSES=build/example.class

all: java

###########################################################################################

src/queryPhraseTableMin_wrap.cxx: queryPhraseTableMin.i
	mkdir -p src build
	swig -java -c++ -I.. -I../src -outdir src -o src/queryPhraseTableMin_wrap.cxx queryPhraseTableMin.i

build/libJavaQueryPhraseTableMin.so: QueryPhraseTableMin.cpp src/queryPhraseTableMin_wrap.cxx ../../lib/*.a
	g++ -Wno-deprecated -shared -fPIC -g -O2 \
	QueryPhraseTableMin.cpp src/queryPhraseTableMin_wrap.cxx \
	../../lib/*.a ../../lib/*.a ../../lib/*.a \
	-I../../moses/src -I../.. \
	-I${JAVA_INCLUDES} -I${JAVA_INCLUDES}/linux \
	-I../src -I.. -I. \
	-lboost_thread -lcmph -lz -lboost_filesystem \
	-o build/libJavaQueryPhraseTableMin.so

build/test: test.cpp
	g++ -Wno-deprecated -g -O2 \
	test.cpp \
	-I../src -I.. -I. \
	-Lbuild -lJavaQueryPhraseTableMin \
	-o build/test

${JAVA_CLASSES}: src/queryPhraseTableMin_wrap.cxx
	javac -d build ${JAVA_SOURCES}
	
java: build/libJavaQueryPhraseTableMin.so build/test ${JAVA_CLASSES}

###########################################################################################

clean:
	rm -rf src build

#	../../lib/libfuzzy-match.a ../../lib/libCYKPlusParser.a ../../lib/libCompactPT.a ../../lib/libOnDiskPt.a ../../lib/libmoses.a ../../lib/libmoses_internal.a ../../lib/libkenutil.a \
#	../../lib/libCompactPT.a ../../lib/libmoses.a ../../lib/libmoses_internal.a ../../lib/libRuleTable.a ../../lib/libOnDiskPt.a ../../lib/libRuleTable.a ../../lib/libOnDiskPt.a ../../lib/libLM.a ../../lib/libkenutil.a ../../lib/libkenlm.a \
#	../../lib/libCompactPT.a ../../lib/libScope3Parser.a ../../lib/libkenutil.a ../../lib/libmoses.a ../../lib/libRuleTable.a ../../lib/libCompactPT.a ../../lib/libmoses_internal.a \
