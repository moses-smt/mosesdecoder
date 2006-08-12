// $Id$

import java.io.*;
import java.util.*;

// create sentences with all features combined from files with individual tags 
class CombineTags
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		Vector vecInstream = new Vector();
		for (int i = 0 ; i < args.length ; i++)
		{
			InputStreamReader temp = new InputStreamReader(new FileInputStream(args[i]), "Latin1");
			BufferedReader inStream = new BufferedReader(temp);
			vecInstream.add(inStream);
		}
		OutputStreamWriter outStream = new OutputStreamWriter((OutputStream)System.out, "Latin1"); 
		
		new CombineTags(vecInstream, outStream);
		
		System.err.println("End...");
	}

	public CombineTags(Vector vecInstream , OutputStreamWriter outStream) throws Exception
	{
		BufferedReader inFile = (BufferedReader) vecInstream.get(0);
		String inLine;
		while ((inLine = inFile.readLine()) != null)
		{
			Vector phrases = new Vector();

			// do 1st stream
			Vector phrase = new Vector();
			StringTokenizer st = new StringTokenizer(inLine);
		     while (st.hasMoreTokens()) 
		     {
		    	 String tag = st.nextToken();
		    	 phrase.add(tag); 
		     }
		     phrases.add(phrase);
		     
			// read other stream
	 		for (int i = 1 ; i < vecInstream.size() ; i++)
			{
	 			BufferedReader otherFile = (BufferedReader) vecInstream.get(i);
	 			String otherLine = otherFile.readLine();
 				StringTokenizer otherSt = new StringTokenizer(otherLine);
 				Vector otherPhrase = new Vector();
 				
 			     while (otherSt.hasMoreTokens()) 
 			     {
 			    	String tag = otherSt.nextToken();
 			    	otherPhrase.add(tag); 
 			     }
 			     phrases.add(otherPhrase);
 			}
	 		
	 		// combine
 			phrase = (Vector) phrases.get(0);
 			
 			for (int pos = 0 ; pos < phrase.size() ; pos++)
 			{
 				String outLine = (String) phrase.get(pos) + "|";
 				
 				for (int stream = 1 ; stream < phrases.size() ; stream++)
 				{
 					Vector otherPhrase = (Vector) phrases.get(stream);
 					String otherTag;
 					if (otherPhrase.size() <= pos)
 						otherTag = (String) otherPhrase.get(0);
 					else
 						otherTag = (String) otherPhrase.get(pos);
 					outLine += otherTag + "|";
 				}
 				outLine = outLine.substring(0, outLine.length() - 1) + " ";
 				outStream.write(outLine);
 			}
	 		outStream.write("\n");
		}
		// close stream
		outStream.flush();
		outStream.close();
		outStream = null;
	}
}

