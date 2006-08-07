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
		
		new ProcessShallowParse(inStream, outStream);
		
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

