// $Id$

import java.io.*;
import java.util.*;

// create pos-tag sentences from LISP-like input tree.
// NN-NK tag augmented with NP-SP if parent is NP-SB
class TagHierarchy
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		InputStreamReader inStream = new InputStreamReader(args.length > 0 ? new FileInputStream(args[0]) : System.in
														, "Latin1"); 
		OutputStreamWriter outStream = new OutputStreamWriter(args.length > 1 ? new FileOutputStream(args[1]) : (OutputStream) System.out
														, "Latin1"); 
		
		new TagHierarchy(inStream, outStream);
		
		System.err.println("End...");
	}

	public TagHierarchy(Reader inStream, OutputStreamWriter outStream) throws Exception
	{
		BufferedReader inFile = new BufferedReader(inStream);
		BufferedWriter outFile = new BufferedWriter(outStream);
		
		// tokenise
		String inLine;
		int nullLines = 0;
		while ((inLine = inFile.readLine()) != null)
		{
			if (inLine.equals("null"))
			{
				nullLines++;
				outFile.write("null\n");
			}
			else
			{
				OutputHierarchy2(inLine, outFile);
			}
		}
		outFile.flush();
		outFile.close();
		outFile = null;
		System.err.println(nullLines + " null lines\n");
	}

	// indent parsed tree to make it easier to look at
	public void OutputHierarchy(String inLine, BufferedWriter outFile) throws Exception
	{
		int level = 0;
		StringTokenizer st = new StringTokenizer(inLine);
	     while (st.hasMoreTokens()) 
	     {
	    	 String parsed = st.nextToken();
	    	 if (parsed.substring(0, 1).compareTo("(") == 0)
	    	 { // start of new node
	    		 outFile.write('\n');
	    		 for (int currLevel = 0 ; currLevel < level ; currLevel++)
	    		 {
	    			 outFile.write(' ');
	    		 }
	    		 String tag = parsed.substring(1, parsed.length());
	    		 outFile.write(tag);
	    		 level++;
	    	 }
	    	 else
	    	 { // closing nodes
	    		 int firstBracket = parsed.indexOf(')');
	    		 int noBracket = parsed.length() - firstBracket;
	    		 String tag = parsed.substring(0, firstBracket);
	    		 outFile.write(" == " + tag);
	    		 level -= noBracket;
	    	 }
	     }
	     outFile.write('\n');
	}

	public void OutputHierarchy2(String inLine, BufferedWriter outFile) throws Exception
	{
		int level = 0;
		Stack prevTags = new Stack();
		
		StringTokenizer st = new StringTokenizer(inLine);
		
	    while (st.hasMoreTokens()) 
	    {
	    	String parsed = st.nextToken();
	    	if (parsed.substring(0, 1).compareTo("(") == 0)
	    	{ // start of new node
	    		String tag = parsed.substring(1, parsed.length());
	    		prevTags.push(tag);
	    		level++;
	    	}
	    	else
	    	{ // closing nodes
	    		
	    		String parentTag = (String) prevTags.get(prevTags.size() - 2)
	    				, currTag = (String) prevTags.get(prevTags.size() - 1);
	    		if (currTag.equals("NN-NK") && parentTag.equals("NP-SB"))
	    			currTag += "_" + parentTag;

	    		int firstBracket = parsed.indexOf(')');
	    		int noBracket = parsed.length() - firstBracket;
	    		String word = parsed.substring(0, firstBracket);

	    		if (currTag.equals("ART-SB")
		    			|| currTag.equals("NN-NK_NP-SB")
		    			|| currTag.equals("VAFIN-HD")
		    			|| currTag.equals("VVFIN-HD")
		    			|| currTag.equals("VMFIN-HD")
		    			|| currTag.equals("PPER-SB")
		    			|| currTag.equals("PRELS-SB")
		    			|| currTag.equals("PDS-SB")
		    			|| currTag.equals("PPER-PH")
		    			|| currTag.equals("PPER-EP")
		    			)
	    			outFile.write(currTag + " ");
	    		else
	    			outFile.write("??? ");
	    		
	    		level -= noBracket;
	    		
	    		// pop the rest
	    		for (int i = 0 ; i < noBracket ; ++i)
	    		{
	    			prevTags.pop();
	    		}
	    	}
	    }
	    outFile.write('\n');
	}
}
