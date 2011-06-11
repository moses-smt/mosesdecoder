package mosesui.app;

import android.app.Activity;
import android.os.Bundle;
import android.text.Editable;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class TranslateActivity extends Activity {
    public static final String TAG = "Translate";

	private Button m_cmdTranslate;
	private EditText m_txtSource, m_txtTarget;

    // Called when the activity is first created. 
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.translate);
        
        m_cmdTranslate = (Button) findViewById(R.id.cmdTranslate2);
        m_txtSource = (EditText) findViewById(R.id.txtSource);
        m_txtTarget = (EditText) findViewById(R.id.txtTarget);
        
        m_cmdTranslate.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	cmdTranslateClicked();
            }
        });
    }
    
    private void cmdTranslateClicked() {
    	Editable source = m_txtSource.getText();

    	JNIWrapper jni = new JNIWrapper();
        
    	String target = jni.translateSentence(source.toString());
//        String target = translateSentence();
//        String target = translateSentence(11);

    	Log.v(TAG, "Translating " + source);
    	Log.v(TAG, "Translating " + target);

    	m_txtTarget.setText(target);

    }


}
