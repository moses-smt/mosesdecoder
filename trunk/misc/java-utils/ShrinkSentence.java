// $Id$

import java.io.*;
import java.util.*;

//used to create language model
public class ShrinkSentence
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		InputStreamReader inStream = new InputStreamReader(args.length > 0 ? new FileInputStream(args[0]) : System.in
														, "Latin1"); 
		OutputStreamWriter outStream = new OutputStreamWriter(args.length > 1 ? new FileOutputStream(args[1]) : (OutputStream) System.out
														, "Latin1"); 
		
		new ShrinkSentence(inStream, outStream);
		
		System.err.println("End...");
	}

	public ShrinkSentence(Reader inStream, Writer outStream) throws Exception
	{
		BufferedReader inFile = new BufferedReader(inStream); 
		BufferedWriter outFile = new BufferedWriter(outStream); 

		// tokenise
		String inLine;
		int i = 1;
		while ((inLine = inFile.readLine()) != null)
		{
			StringTokenizer st = new StringTokenizer(inLine);
			while (st.hasMoreTokens()) 
		    {
				String word = st.nextToken();
				if (!word.equals("???"))
					outFile.write(word + " ");
		    }
			outFile.write("\n");
			i++;
		}
		outFile.flush();
		outFile.close();
		outFile = null;
		System.err.print("no of lines = " + i);		
	}
}