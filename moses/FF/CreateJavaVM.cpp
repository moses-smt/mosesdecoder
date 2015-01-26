#include "CreateJavaVM.h"
#include "moses/StaticData.h"


using namespace std;

namespace Moses{

// Global static pointer used to ensure a single instance of the class.
CreateJavaVM* CreateJavaVM::m_pInstance = NULL;

/** This function is called to create an instance of the class.
 Calling the constructor publicly is not allowed.
 The constructor is private and is only called by this Instance function.
*/
CreateJavaVM* CreateJavaVM::Instance(std::string jarPath){
	if(!m_pInstance)
		m_pInstance = new CreateJavaVM(jarPath);

	return m_pInstance;
}

CreateJavaVM::CreateJavaVM(std::string jarPath){
	int nArgs=4;
	JavaVMOption args[nArgs]; //* args = new JavaVMOption[nArgs];
	args[0].optionString = "-verbose:gc,class,jni";

	string path = "-Djava.class.path="+jarPath;
	args[1].optionString= const_cast<char*>(path.c_str());//"-Djava.class.path=/Users/mnadejde/Documents/workspace/stanford-parser-full-2014-08-27/stanford-parser-3.4.1-models.jar:/Users/mnadejde/Documents/workspace/stanford-parser-full-2014-08-27/stanford-parser.jar:/Users/mnadejde/Documents/workspace/stanford-parser-full-2014-08-27/commons-lang3-3.3.2.jar:/Users/mnadejde/Documents/workspace/moses_010914/mosesdecoder/Relations.jar";
	args[2].optionString= "-d64";
	args[3].optionString= "-Xmx500m";
	//add this when debuging the java calls (otherwise it makes stuff slower)
	//args[4].optionString= "-Xcheck:jni";

	JavaVMInitArgs vm_args;
	vm_args.version = JNI_VERSION_1_6;
	vm_args.nOptions = nArgs;
  vm_args.options = args;
  vm_args.ignoreUnrecognized = JNI_TRUE;

  // Construct a VM
  JNIEnv *env; // keep this pointer local to the constructor as it is only valid in a thread anyway
  jint res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);

  VERBOSE(1, "JNI_CreateJavaVM result: jint=" << res << std::endl);
  jclass Relations = env->FindClass("Relations");
  if(Relations==NULL)
  	cerr<<"Relations class NULL"<<endl;
  env->ExceptionDescribe();
	//keep this pointer throughout decoding
  this->relationsJClass = reinterpret_cast <jclass> (env->NewGlobalRef(Relations));
  env->DeleteLocalRef(Relations);

  if (!env->ExceptionCheck())
    {
  		//Note that jfieldIDs and jmethodIDs are opaque types, not object references, and should not be passed to NewGlobalRef.
  		//IDs returned for a given class don't change for the lifetime of the JVM process

  		//Get method ID for: object initializer
  		this->DepParsingInitJId = env->GetMethodID(this->relationsJClass, "<init>","()V");
      if(this->DepParsingInitJId==NULL)
      	cerr<<"constructor MethodID NULL"<<endl;
      env->ExceptionDescribe();

      //Get method ID for: list of selected dep rel types method
      this->GetRelationListJId = env->GetMethodID(this->relationsJClass, "GetRelationList",
                                  "()[Ljava/lang/String;");
      if(this->GetRelationListJId==NULL)
      	cerr<<"GetRelationList MethodID NULL"<<endl;
      env->ExceptionDescribe();

      //Get method ID for: process the sentence and return dependencies method
      this->ProcessParsedSentenceJId = env->GetMethodID(this->relationsJClass, "ProcessParsedSentence",
                            "(Ljava/lang/String;Z)Ljava/lang/String;");
      if(this->ProcessParsedSentenceJId==NULL)
        cerr<<"ProcessParsedSentence MethodID NULL"<<endl;
      env->ExceptionDescribe();

      VERBOSE(1, "JNI finished getting method IDs. ProcessParsedSentenceJId: " << this->ProcessParsedSentenceJId << std::endl);

    }

  	//I don't really understand when/where I should detach and attach
  	vm->DetachCurrentThread();
  	//Can't do that because the class calls: JNIEnv* GetAttachedJniEnvPointer();
  	//vm->DestroyJavaVM();
  	//VERBOSE(1, "JavaCppJni instance destroyed." << std::endl);
  	//this->TestRuntime();
}

//From Dominikus
// Help: http://stackoverflow.com/questions/12900695/how-to-obtain-jni-interface-pointer-jnienv-for-asynchronous-calls
JNIEnv* CreateJavaVM::GetAttachedJniEnvPointer()
{
	//So JNIEnv *env it already a pointer to a pointer that's why I can return it as a parameter
	//https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/design.html
	//Each function is accessible at a fixed offset through the JNIEnv argument.
	//The JNIEnv type is a pointer to a structure storing all JNI function pointers. It is defined as follows:
	//typedef const struct JNINativeInterface *JNIEnv;
  JNIEnv *env;
  int getEnvStat = this->vm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (getEnvStat == JNI_EDETACHED) {
     // VERBOSE(1, "GetEnv: not attached. Attempting to attach ... ");
      if (vm->AttachCurrentThread((void **)&env, NULL) != 0) {
          VERBOSE(1, "Failed to attach" << std::endl);
      }
/*      else {
          VERBOSE(1, "Attached successfully." << std::endl);
      }
      */
  } else if (getEnvStat == JNI_OK) {
      //
  } else if (getEnvStat == JNI_EVERSION) {
      VERBOSE(1, "GetEnv: version not supported" << std::endl);
  }
  // FIXME aren't we returning a local pointer? why does this work?
  return env;
}

void CreateJavaVM::GetDep(std::string parsedSentence){
	JNIEnv *env = GetAttachedJniEnvPointer();
	//Local references become invalid when the execution returns from the native method where the local ref was created
		// Probably want a global reference? -> to cache?
		jobject rel = env->NewObject(this->relationsJClass, this->DepParsingInitJId);
		env->ExceptionDescribe();
		//this reference remains valid until it is explicity freed -> prevents class from being unloaded
		//use this object and just call ProcessParsedSentence() with new arguments for each hypothesis
		jobject relGlobal = env->NewGlobalRef(rel);
		env->DeleteLocalRef(rel);

		//Get method ID for: process the sentence and return dependencies method
		      this->ProcessParsedSentenceJId = env->GetMethodID(this->relationsJClass, "ProcessParsedSentence",
		                            "(Ljava/lang/String;Z)Ljava/lang/String;");
		      if(this->ProcessParsedSentenceJId==NULL)
		        cerr<<"ProcessParsedSentence MethodID NULL"<<endl;
		      env->ExceptionDescribe();

		//Get list of selected dep relations
		/**
		jobject relationsArray = env ->CallObjectMethod(relGlobal,this->GetRelationListJId);
		env->ExceptionDescribe();
		cerr<<"got relationsArray"<<endl;
		**/

		//string sentence = "(S (NP (NNP Sam)) (VP (VBD died) (NP-TMP (NN today))))";
		string sentence ="(VP (VB give)(PP (DT a)(JJ separate)(NNP GC)(NN exam)))";
		cerr<<"IN TEST: "<<sentence<<endl;
		jstring jSentence = env->NewStringUTF(sentence.c_str());
		//jstring jSentence = env->NewStringUTF(parsedSentence.c_str());
		jboolean jSpecified = JNI_TRUE;

		//test memory

		//make the java method synchronized ? di i need to -> one object for sentence? -> initialized when feature is loaded??
		//SHOULT HAVE ONE OBJECT PERS SENTENCE AND THEN DELETE SO THE MEMORY GETS FREED -> does it get free?

		//MAKE THIS PROCESS FUNCTION RETURN SOMETHING OTHERWISE IT MIGHT CRASH ?
		jobject jStanfordDepObj = env ->CallObjectMethod(relGlobal,this->ProcessParsedSentenceJId,jSentence,jSpecified);


		//jobject jStanfordDepObj = env ->CallObjectMethod(relGlobal,ProcessParsedSentence,jSentence,jSpecified);
		//PROBLEM when calling the method ???
		//jstring jStanfordDep = reinterpret_cast <jstring> (env ->CallObjectMethod(relGlobal,ProcessParsedSentence,jSentence,jSpecified));
		env->ExceptionDescribe();
		jstring jStanfordDep = reinterpret_cast <jstring>(jStanfordDepObj);
		const char* stanfordDep = env->GetStringUTFChars(jStanfordDep, 0);
		cerr << "TEST DEP: "<< stanfordDep <<endl;
		env->ReleaseStringUTFChars(jStanfordDep, stanfordDep);

		//how to make sure the memory gets released on the Java side?
		env->DeleteLocalRef(jStanfordDep);
		env->DeleteLocalRef(jStanfordDepObj);

		//have to figure how to manage this object
		env->DeleteGlobalRef(relGlobal);
		this->vm->DetachCurrentThread();
	//return const_cast<char*>(stanfordDep);
}

void CreateJavaVM::TestRuntime(){
	JNIEnv *env = GetAttachedJniEnvPointer();
	VERBOSE(1, "START querying dep rel " << std::endl);
		std::clock_t    start,mid;
		start = std::clock();
		mid=start;
		for(int i=0;i<10000;i++){

	//Local references become invalid when the execution returns from the native method where the local ref was created
	// Probably want a global reference? -> to cache?
	jobject rel = env->NewObject(this->relationsJClass, this->DepParsingInitJId);
	env->ExceptionDescribe();
	//this reference remains valid until it is explicity freed -> prevents class from being unloaded
	//use this object and just call ProcessParsedSentence() with new arguments for each hypothesis
	jobject relGlobal = env->NewGlobalRef(rel);
	env->DeleteLocalRef(rel);

	//Get list of selected dep relations
	/**
	jobject relationsArray = env ->CallObjectMethod(relGlobal,this->GetRelationListJId);
	env->ExceptionDescribe();
	cerr<<"got relationsArray"<<endl;
	**/

	//!!! Gives error if string is empty
	//string sentence = "(S (NP (NNP Sam)) (VP (VBD died) (NP-TMP (NN today))))";
	string sentence = "(NN exam)";
	//string sentence ="( (S (NP (JJ English) (NN literature) (NNS courses)) (VP (MD will) (VP (VB require) (S (NP (NNS pupils)) (VP (TO to) (VP (VB study) (NP (NP (QP (IN at) (JJS least) (CD one)) (NNP Shakespeare) (NN play)) (, ,) (NP (DT a) (JJ 19th) (NN century) (NN novel)) (, ,) (NP (JJ Romantic) (NN poetry)) (CC and) (NP (NP (JJ contemporary) (JJ British) (NN fiction)) (PP (IN from) (NP (CD 1914) (NNS onwards)))))))))) (. .)) )";
	jstring jSentence = env->NewStringUTF(sentence.c_str());
	jboolean jSpecified = JNI_TRUE;
	env->ExceptionDescribe();
	//test memory
	VERBOSE(1, "trying to call method ProcessParsedSentenceJId" << std::endl);

	//make the java method synchronized ? di i need to -> one object for sentence? -> initialized when feature is loaded??
	//SHOULT HAVE ONE OBJECT PERS SENTENCE AND THEN DELETE SO THE MEMORY GETS FREED -> does it get free?
	jobject jStanfordDepObj = env ->CallObjectMethod(relGlobal,this->ProcessParsedSentenceJId,jSentence,jSpecified);
	//jobject jStanfordDepObj = env ->CallObjectMethod(relGlobal,ProcessParsedSentence,jSentence,jSpecified);
	//PROBLEM when calling the method ???
	//jstring jStanfordDep = reinterpret_cast <jstring> (env ->CallObjectMethod(relGlobal,ProcessParsedSentence,jSentence,jSpecified));
	env->ExceptionDescribe();
	jstring jStanfordDep = reinterpret_cast <jstring>(jStanfordDepObj);
	const char* stanfordDep = env->GetStringUTFChars(jStanfordDep, 0);
	//cerr << stanfordDep;
	env->ReleaseStringUTFChars(jStanfordDep, stanfordDep);

	//how to make sure the memory gets released on the Java side?
	env->DeleteLocalRef(jStanfordDep);
	env->DeleteLocalRef(jStanfordDepObj);
	if(i%10000==0){
		VERBOSE(1, i << " "<< (std::clock() - mid) / (double)(CLOCKS_PER_SEC) << std::endl);
		mid = std::clock();
	}
	//have to figure how to manage this object
	env->DeleteGlobalRef(relGlobal);
		}
		VERBOSE(1, "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC) << " s" << std::endl);
			VERBOSE(1, "STOP querying dep rel " << std::endl);
	this->vm->DetachCurrentThread();
}



}
