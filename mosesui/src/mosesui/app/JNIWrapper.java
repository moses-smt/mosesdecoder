package mosesui.app;

public class JNIWrapper {

    // jni
	  public native int loadModel(
				String appPath
				, String iniPath
				, String source
				, String target
				, String description);
	  public native String  translateSentence(String source);
//	    public native String  translateSentence(int source);
//	    public native String  translateSentence();

}
