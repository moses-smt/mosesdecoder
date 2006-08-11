// $Id$

import java.io.*;
import java.util.*;

class TagHierarchy
{
	public static void main(String[] args) throws Exception
	{
		System.err.println("Starting...");

		InputStreamReader inStream = args.length > 0 ? new FileReader(args[0]) : new InputStreamReader(System.in); 
		PrintStream outStream = args.length > 1 ? new PrintStream(new File(args[1])) : System.out; 
		
		new TagHierarchy(inStream, outStream);
		
		System.err.println("End...");
	}

	public TagHierarchy(Reader inStream, PrintStream outStream) throws Exception
	{
		BufferedReader inFile = new BufferedReader(inStream); 
		
		// tokenise
		String inLine;
		int nullLines = 0;
		while ((inLine = inFile.readLine()) != null)
		{
			if (inLine.equals("null"))
			{
				nullLines++;
				outStream.println("null");
			}
			else
			{
				OutputHierarchy2(inLine, outStream);
			}
		}
		System.err.println(nullLines + " null lines\n");
	}

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

	public void OutputHierarchy2(String inLine, PrintStream outFile) throws Exception
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
	    		outFile.print(currTag + " ");
	    		
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
