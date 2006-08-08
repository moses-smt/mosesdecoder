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
		while ((inLine = inFile.readLine()) != null)
		{
			StringTokenizer st = new StringTokenizer(inLine);
		     while (st.hasMoreTokens()) 
		     {
		    	 String factoredWord = st.nextToken();
		    	 Output(factoredWord, outFile);
		     }
		     outFile.write("\n");
		}		
	}
	
	protected void Output(String factoredWord, BufferedWriter outStream) throws Exception
	{
		StringTokenizer st = new StringTokenizer(factoredWord, "|");
    	st.nextToken();
    	String pos = st.nextToken();
    	String morph = st.nextToken();

    	int lastPos = pos.lastIndexOf('-');
    	
    	if (pos.indexOf("ART") == 0
    		|| pos.indexOf("P") == 0
    		|| pos.indexOf("V") == 0
    		|| pos.indexOf("$,") == 0
    		|| pos.indexOf("$.") == 0
    		)
    	{
    		outStream.write(pos + "|" + morph + " ");
    	}
	}
}
