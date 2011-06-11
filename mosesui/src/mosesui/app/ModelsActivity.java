package mosesui.app;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
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
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class ModelsActivity extends Activity {
    public static final String TAG = "Models";
	private Button m_cmdDownload;
	private EditText m_txtURL, m_txtProgress;
	private ListView m_listModels;
	private Vector<File> m_storage = new Vector<File>();
	
	private Vector<String> m_arrModels = new Vector<String>(); 
	private ArrayAdapter<String> m_listAdapter;
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.models);

        m_cmdDownload = (Button) findViewById(R.id.cmdDownload);
        m_txtURL = (EditText) findViewById(R.id.txtURL);
        m_listModels = (ListView) findViewById(R.id.listModels);
        m_txtProgress = (EditText) findViewById(R.id.txtProgress);
        
        m_cmdDownload.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	cmdDownloadClicked();
            }
        });
        
        OnItemClickListener listener = new OnItemClickListener() {
            public void onItemClick(AdapterView av, View v, int i, long l) {
            	listModelItemSelected(i);            	
            }
		};
        m_listModels.setOnItemClickListener(listener);
        
        m_listAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1 , m_arrModels);
        m_listModels.setAdapter(m_listAdapter);

        // create folders, in case its the 1st time
        File storageDir = getExternalFilesDir(null);
    	if (storageDir != null)
    		m_storage.add(storageDir);
    	
    	storageDir = getFilesDir();
    	if (storageDir != null)
    		m_storage.add(storageDir);
    	
    	assert(m_storage.size() > 0);
    	storageDir = m_storage.get(0);

    	File zipDir = new File(storageDir + "/zip/");
    	boolean ret = zipDir.mkdirs();
    	
    	File modelDir = new File(storageDir + "/models/");
    	ret = modelDir.mkdirs();	

    	listModels();  
    	
	}

	private void enableTranslate(boolean enable)
	{
		MosesUI tabs = (MosesUI) this.getParent();
		Log.v(TAG, tabs.toString());
		if (enable)
		{
			tabs.getTabHost().setCurrentTab(0);
		}
		else
		{

		}
	}
	
	private void listModels()
	{
    	m_arrModels.clear(); 
		for (int i = 0; i < m_storage.size(); ++i)
		{
			File storageDir = m_storage.get(i);
			listModels(i, storageDir);
		}
	}
	
	private void listModels(int ind, File storageDir)
	{
    	File modelDir = new File(storageDir + "/models/");

    	String[] modelPaths = modelDir.list();
    	if (modelPaths == null)
    		return;
    	
    	for (int i = 0; i < modelPaths.length; ++i)
    	{
    		String modelName = modelPaths[i];
    		File dir = new File(modelDir + "/" + modelName);
    		if (dir.isDirectory())
    		{
    			File iniPath = new File(dir + "/moses.ini");
    			if (iniPath.exists())
    			{
    				modelName = ind + modelName;
    	    		Log.v(TAG, modelName);
    	    		m_arrModels.add(modelName);
    			}
    		}
    	}    	
	}
	
	private void loadModel(int storageIndex, String model)
	{
    	Log.v(TAG, "Load " + model);
        File storageDir = m_storage.get(storageIndex);
    	File modelDir = new File(storageDir + "/models/");

		String appPath = modelDir + "/" + model;
		String iniPath = appPath + "/moses.ini";
		String source = "SSS";
		String target = "TTT";
		String descr = "DDDD";

		JNIWrapper jni = new JNIWrapper();
		int ret = jni.loadModel(appPath, iniPath, source, target, descr);
    	Log.v(TAG, Integer.toString(ret));
    	m_txtProgress.setText(Integer.toString(ret));

    	enableTranslate(ret == 0);
    	
	}
	
	private void listModelItemSelected(int i) {	
    	String model = m_arrModels.get(i);
    	int storageIndex = Integer.parseInt(model.substring(0, 1));
    	model = model.substring(1, model.length());
    	
    	loadModel(storageIndex, model);
	}

	private void cmdDownloadClicked() {
    	Editable url = m_txtURL.getText();

        File storageDir = m_storage.get(0);
        File zipDir = new File(storageDir + "/zip/");
    	File modelDir = new File(storageDir + "/models/");
    	
    	DownloadZip downloadZip = new DownloadZip();
    	downloadZip.execute(url.toString(), zipDir.getPath(), modelDir.getPath());
    	
    }	

	private class DownloadZip extends AsyncTask<String, String, Void>
	{
		File m_file = null;
		File m_modelDir = null;
		
		protected void onProgressUpdate (String... values)
		{
			for (int i = 0; i < values.length; ++i)
			{
				String value = values[i];
				if (value.equals("OK"))
				{
			    	Unzip zipFile = new Unzip();
			    	zipFile.execute(m_file.getPath(), m_modelDir.getPath());
				}
				m_txtProgress.setText("Downloaded "  + values[i]);
			}
		}

		protected Void doInBackground(String...args) 
		{
			assert(args.length == 3);
			String url = args[0];
			File zipDir = new File(args[1]);
			m_modelDir = new File(args[2]);
			
	    	try {	
		    	URL u = new URL(url.toString());
		        HttpURLConnection c = (HttpURLConnection) u.openConnection();
		        c.setRequestMethod("GET");
		        c.setDoOutput(true);
		        c.connect();
		
		        InputStream in = c.getInputStream();
		
		        m_file = File.createTempFile("moses-ui", null, zipDir);
		        FileOutputStream f = new FileOutputStream(m_file);

		        byte[] buffer = new byte[1024];
		        int len = 0, totalLen = 0, count = 0;
		        while ( (len = in.read(buffer)) > 0 ) {
		            f.write(buffer,0, len);
		            totalLen += len;
		            
		            if (count % 100 == 0)
		            {
                        publishProgress(Integer.toString(totalLen));
		            }
		            ++count;
		            
		        }
		        f.close();
	    	}
	    	catch (Exception ex) {
                publishProgress(ex.toString());
	    	}
	    	
	    	publishProgress("OK");
			return null;
		}
	}
	
	private class Unzip extends AsyncTask<String, String, Void>
	{
		protected void onProgressUpdate (String... values)
		{
			for (int i = 0; i < values.length; ++i)
			{
				String value = values[i];
				if (value.equals("OK"))
				{
					listModels();
			    	m_listAdapter.notifyDataSetChanged();
				}
				m_txtProgress.setText(values[i]);
			}
		}

		protected Void doInBackground(String...args) 
		{
			assert(args.length == 2);
			String zipPath = args[0];
			String outputDir = args[1];
			File archive = new File(zipPath);
			
	        Message msg;
	        try {
	                ZipFile zipfile = new ZipFile(archive);
	                for (Enumeration<? extends ZipEntry> e = zipfile.entries(); e.hasMoreElements();) {
	                        ZipEntry entry = (ZipEntry) e.nextElement();
	                        String str = "Extracting " + entry.getName();
	                        publishProgress(str);
	                        
	                        unzipEntry(zipfile, entry, outputDir);
	                }
	        } catch (Exception e) {
	            publishProgress(e.toString());
	        }

            publishProgress("OK");
			return null;
		}
		
	    private void unzipEntry(ZipFile zipfile, ZipEntry entry,
	            String outputDir) throws IOException {

	    if (entry.isDirectory()) {
	            createDir(new File(outputDir, entry.getName()));
	            return;
	    }

	    File outputFile = new File(outputDir, entry.getName());
	    if (!outputFile.getParentFile().exists()) {
	            createDir(outputFile.getParentFile());
	    }

	    publishProgress("Extracting: " + entry);
	    BufferedInputStream inputStream = new BufferedInputStream(zipfile.getInputStream(entry));
	    BufferedOutputStream outputStream = new BufferedOutputStream(new FileOutputStream(outputFile));

	    try {
	            IOUtils.copy(inputStream, outputStream);
	    } finally {
	            outputStream.close();
	            inputStream.close();
	    }
	  }

	  private void createDir(File dir) 
	  {
	    publishProgress("Creating dir " + dir.getName());
	    if (!dir.mkdirs())
	    {} //        throw new RuntimeException("Can not create dir " + dir);
	  }

		
	} // unzip 2 clasdd
}

