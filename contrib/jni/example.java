// ptQuery.java

import java.io.*;

public class example {
    static {
	System.loadLibrary("javaforcealigner");
    }

    public static void main(String argv[]) {
	String mosesModelPath = argv[0];
        String srcLang = argv[1];
        String trgLang = argv[2];
        
        SymForceAligner fa = new SymForceAligner(srcLang, trgLang, mosesModelPath);
	fa.setMode(SymForceAligner.Mode.GrowDiagFinalAnd);
	
	if(fa != null && !fa.errorOccurred()) {
	    try {
		int c = 0;
		BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
		String s = "";
		String srcLine = "";
		String trgLine = "";
		while ((s = in.readLine()) != null && s.length() != 0) {
		    if(c % 2 == 0) {
			srcLine = s;
		    }
		    else {
			trgLine = s;
			
			String a = fa.alignSentenceStr(srcLine, trgLine);
			System.out.print(a + "\n");
		    }
		    c++;
		}
	    } catch (IOException e) {}
	}
    }
}
