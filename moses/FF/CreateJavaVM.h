
//this doesn't have version 1.8 -> have to reinstall jdk 1.6
#include </System/Library/Frameworks/JavaVM.framework/Headers/jni.h>
//#include </System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers/jni.h>
//#include <jni.h>
//this doesn't compile
//#include </Library/Java/JavaVirtualMachines/jdk1.8.0_25.jdk/Contents/Home/include/jni.h>

#include <string>

namespace Moses{

class CreateJavaVM{
//could make these static so they only instantiate once?
	//-> in principle this class will be instantiated once when loading the feature
private:
  JavaVM *vm;
  jclass relationsJClass;
  //StanfordDep object initializer method ID
  jmethodID DepParsingInitJId;
  //list of selected dep rel types method ID
  jmethodID GetRelationListJId;
  //process the sentence and return dependencies method ID
  jmethodID ProcessParsedSentenceJId;

  static CreateJavaVM *m_pInstance;


public:
  ~CreateJavaVM();
  CreateJavaVM(std::string jarPath);
  void TestRuntime();
  JNIEnv* GetAttachedJniEnvPointer();
  //help make the class a singleton
  static CreateJavaVM* Instance(std::string jarPath);

  JavaVM* GetVM(){return vm;}
  jclass GetRelationsJClass(){return relationsJClass;}
  jmethodID GetDepParsingInitJId(){return DepParsingInitJId;}
  jmethodID GetGetRelationListJId(){return GetRelationListJId;}
  jmethodID GetProcessParsedSentenceJId(){return ProcessParsedSentenceJId;}
  void GetDep(std::string parsedSentence);

};
}
