JAVA_INCLUDES=/usr/lib/jvm/java-7-openjdk-amd64/include

JAVA_SOURCES=example.java exampleThreads.java src/javaforcealignerJNI.java src/ForceAligner.java src/SymForceAligner.java src/Corpus.java
JAVA_CLASSES=build/example.class build/exampleThreads.class

all: java

###########################################################################################

src/javaforcealigner_wrap.cxx: javaforcealigner.i
	mkdir -p src build
	swig -java -c++ -I../src -outdir src -o src/javaforcealigner_wrap.cxx javaforcealigner.i

build/libjavaforcealigner.so: src/javaforcealigner_wrap.cxx ../src/libgiza.a
	g++ -Wno-deprecated -shared -Xlinker -zmuldefs -fPIC \
	src/javaforcealigner_wrap.cxx \
	../src/libgiza.a \
	-I${JAVA_INCLUDES} -I${JAVA_INCLUDES}/linux \
	-I../src \
	-o build/libjavaforcealigner.so
	
${JAVA_CLASSES}: src/javaforcealigner_wrap.cxx
	javac -d build ${JAVA_SOURCES}
	
java: build/libjavaforcealigner.so ${JAVA_CLASSES}

###########################################################################################

clean:
	rm -rf src build