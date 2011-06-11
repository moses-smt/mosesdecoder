package mosesui.app;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyStore.LoadStoreParameter;
import java.util.Enumeration;
import java.util.List;
import java.util.Vector;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.apache.commons.io.IOUtils;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Message;
import android.text.Editable;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class MiscActivity extends Activity {
    public static final String TAG = "Misc";
	private Button m_cmdDelAllModels, m_cmdShowLog, m_cmdDeleteLog;
	
	private WebView a;
	private Vector<String> m_arrModels = new Vector<String>(); 
	private ArrayAdapter<String> m_listAdapter;
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.misc);

        //a = (WebView) findViewById(R.id.webView1);        
        //a.loadUrl("http://www.asiaonline.net");

        m_cmdDelAllModels = (Button) findViewById(R.id.cmdDeleteAllModels);
        m_cmdShowLog = (Button) findViewById(R.id.cmdShowLog);
        m_cmdDeleteLog = (Button) findViewById(R.id.cmdDeleteLog);

        m_cmdDelAllModels.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	cmdDeleteAllModelsClicked();
            }
        });
        m_cmdShowLog.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	cmdShowLogClicked();
            }
        });
        m_cmdDeleteLog.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	cmdDeleteLogClicked();
            }
        });
        
	}

	private void cmdDeleteLogClicked()
	{
    	File logFile = new File("/mnt/sdcard/Android/data/mosesui.app/files/log.txt");
    	logFile.delete();
	}
	
	private void cmdShowLogClicked()
	{
		try {
		  FileInputStream fstream = new FileInputStream("/mnt/sdcard/Android/data/mosesui.app/files/log.txt");
		  DataInputStream in = new DataInputStream(fstream);
		  BufferedReader br = new BufferedReader(new InputStreamReader(in));
		  StringBuffer buf = new StringBuffer();
		  String strLine;
		  //Read File Line By Line
		  while ((strLine = br.readLine()) != null)   {
		  // Print the content on the console
			  buf.append(strLine + "\n");
		  }
		  
		  EditText txtOutput = (EditText) findViewById(R.id.txtOutput);
		  txtOutput.setText(buf.toString());
		}
		catch (Exception ex) {
			System.err.println(ex.toString());
		}
	}
	
	private void cmdDeleteAllModelsClicked() {

        // create folders, in case its the 1st time
        File storageDir = getExternalFilesDir(null);
    	if (storageDir != null)
    	{
        	deleteRecursive(storageDir);
    	}
    	
    	storageDir = getFilesDir();
    	if (storageDir != null)
    	{
        	deleteRecursive(storageDir);
    	}
	}
	
	void deleteRecursive(File dir)
	{
		Log.d("DeleteRecursive", "DELETEPREVIOUS TOP" + dir.getPath());
        if (dir.isDirectory())
        {
            String[] children = dir.list();
            for (int i = 0; i < children.length; i++) 
            {
               File temp =  new File(dir, children[i]);
               if(temp.isDirectory())
               {
                   Log.d("DeleteRecursive", "Recursive Call" + temp.getPath());
                   deleteRecursive(temp);
               }
               else
               {
                   Log.d("DeleteRecursive", "Delete File" + temp.getPath());
                   boolean b = temp.delete();
                   if(b == false)
                   {
                       Log.d("DeleteRecursive", "DELETE FAIL");
                   }
               }
            }

            dir.delete();
        }
	}
}

