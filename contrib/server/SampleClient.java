//
// Java Sample client for mosesserver (Created by Marwen AZOUZI)
// The XML-RPC libraries are available at Apache (http://ws.apache.org/xmlrpc/) 
//

import java.util.HashMap;
import java.net.URL;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;

public class SampleClient {
	public static void main(String[] args) {
		try {
			// Create an instance of XmlRpcClient
			XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();
			config.setServerURL(new URL("http://localhost:8080/RPC2"));
			XmlRpcClient client = new XmlRpcClient();
			client.setConfig(config);
			// The XML-RPC data type used by mosesserver is <struct>. In Java, this data type can be represented using HashMap.
			HashMap<String,String> mosesParams = new HashMap<String,String>();
			String textToTranslate = new String("some text to translate .");
			mosesParams.put("text", textToTranslate);
			mosesParams.put("align", "true");
			mosesParams.put("report-all-factors", "true");
			// The XmlRpcClient.execute method doesn't accept Hashmap (pParams). It's either Object[] or List. 
			Object[] params = new Object[] { null };
			params[0] = mosesParams;
			// Invoke the remote method "translate". The result is an Object, convert it to a HashMap.
			HashMap result = (HashMap)client.execute("translate", params);
                        // Print the returned results
			String textTranslation = (String)result.get("text");
			System.out.println("Input : "+textToTranslate);
			System.out.println("Translation : "+textTranslation);
			if (result.get("align") != null){ 
				Object[] aligns = (Object[])result.get("align");
				System.out.println("Phrase alignments : [Source Start:Source End][Target Start]"); 
				for ( Object element : aligns) {
	                		HashMap align = (HashMap)element;	
					System.out.println("["+align.get("src-start")+":"+align.get("src-end")+"]["+align.get("tgt-start")+"]");
				}
			}				
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
