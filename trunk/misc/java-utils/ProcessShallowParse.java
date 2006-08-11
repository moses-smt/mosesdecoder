// $Id$


import java.io.*;
import java.util.*;

public class ProcessShallowParse
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		InputStreamReader inStream = args.length > 0 ? new FileReader(args[0]) : new InputStreamReader(System.in); 
		OutputStreamWriter outStream = args.length > 1 ?  new FileWriter(args[1]) : new OutputStreamWriter(System.out); 
		
		new ProcessShallowParse2(inStream, outStream);
		
		System.err.println("End...");
	}

	public ProcessShallowParse(Reader inStream, Writer outStream) throws Exception
	{
		BufferedReader inFile = new BufferedReader(inStream); 
		BufferedWriter outFile = new BufferedWriter(outStream); 
		
		// tokenise
		String inLine;
		while ((inLine = inFile.readLine()) != null)
		{
			StringTokenizer st = new StringTokenizer(inLine);
		     while (st.hasMoreTokens()) 
		     {
		    	 String token = st.nextToken();
		    	 if (token.substring(0, 2).compareTo("I-") != 0)
		    		 outFile.write(token + " ");
		     }
		     outFile.write("\n");
		}		
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
			outFile.write(i++ + " " + ret);
			if (ret.length() > 0)
				outFile.write("\n");
		}
	}
	
	protected String Output(String factoredWord) throws Exception
	{
		StringTokenizer st = new StringTokenizer(factoredWord, "|");
		
    	String surface = st.nextToken();
    	String posNormal = st.nextToken();
    	String morph = st.nextToken();
    	String posImproved = st.nextToken();
    	String ret = "";

    	if (posImproved.indexOf("ART-SB") == 0
    		|| posImproved.indexOf("NN-NK_NP-SB") == 0)
    	{
    		ret = posImproved + "|" + morph + " ";
    	}
    	else if (posImproved.indexOf("VAFIN-HD") == 0
    			|| posImproved.indexOf("VVFIN-HD") == 0
    			|| posImproved.indexOf("VMFIN-HD") == 0
        		|| posImproved.indexOf("PPER-SB") == 0
        		|| posImproved.indexOf("PRELS-SB") == 0
        		|| posImproved.indexOf("PDS-SB") == 0
        		|| posImproved.indexOf("PPER-PH") == 0
        		|| posImproved.indexOf("PPER-EP") == 0
        	)
    	{
    		ret = posImproved + "|" + surface + " ";
    	}
    	
    	return ret;
	}
}
