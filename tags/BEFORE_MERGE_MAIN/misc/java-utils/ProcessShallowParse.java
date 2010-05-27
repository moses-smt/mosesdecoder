// $Id$


import java.io.*;
import java.util.*;

//input is the sentences with all features combined 
//output sentences combination of morphology, lopar tags and parsed tags
// used to create generation table
public class ProcessShallowParse
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		InputStreamReader inStream = new InputStreamReader(args.length > 0 ? new FileInputStream(args[0]) : System.in
														, "Latin1"); 
		OutputStreamWriter outStream = new OutputStreamWriter(args.length > 1 ? new FileOutputStream(args[1]) : (OutputStream) System.out
														, "Latin1"); 
		
		new ProcessShallowParse2(inStream, outStream);
		
		System.err.println("End...");
	}
}

class ProcessShallowParse2
{ // factored sentence
	
	public ProcessShallowParse2(Reader inStream, Writer outStream) throws Exception
	{		
		BufferedReader inFile = new BufferedReader(inStream); 
		BufferedWriter outFile = new BufferedWriter(outStream); 
		
		// tokenise
		String inLine;
		int i = 1;
		while ((inLine = inFile.readLine()) != null)
		{
			StringTokenizer st = new StringTokenizer(inLine);
			String ret = "";
			while (st.hasMoreTokens()) 
		    {
				String factoredWord = st.nextToken();
		    	ret += Output(factoredWord);
		    }
			outFile.write(ret + "\n");
			i++;
		}
		outFile.flush();
		outFile.close();
		outFile = null;
		System.err.print("no of lines = " + i);
	}
	
	protected String Output(String factoredWord) throws Exception
	{
		StringTokenizer st = new StringTokenizer(factoredWord, "|");
		
    	String surface = st.nextToken();
    	String posNormal = st.nextToken();
    	String morph = st.nextToken();
    	String posImproved = st.nextToken();
    	String ret = "";

    	if (posImproved.equals("ART-SB")
    		|| posImproved.equals("NN-NK_NP-SB"))
    	{
    		ret = posImproved + "_" + morph + " ";
    	}
    	else if (posImproved.equals("???"))
    	{
    		ret = "??? ";
    	}
    	else
    	{
    		ret = surface + " ";
    	}
    	
    	return ret;
	}
}
