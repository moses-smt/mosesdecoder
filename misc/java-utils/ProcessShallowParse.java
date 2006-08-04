

import java.io.*;
import java.util.*;

public class ProcessShallowParse
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		String inputPath = args[0]
		       ,outputPath = args[1];
		new ProcessShallowParse(inputPath, outputPath);
		
		System.err.println("End...");
	}

	public ProcessShallowParse(String inputPath, String outputPath) throws Exception
	{
		BufferedReader inFile = new BufferedReader(new FileReader(inputPath)); 
		BufferedWriter outFile = new BufferedWriter(new FileWriter(outputPath)); 

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

