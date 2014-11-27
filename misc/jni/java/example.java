// ptQuery.java

import java.io.*;

public class example {
    
    static {
        System.loadLibrary("JniQueryPt");
    }

    public static void main(String argv[]) {
        String mosesModelPath = argv[0];
       int scores = 5;    
       if(argv.length > 1)
          scores = Integer.parseInt(argv[1]);
    
        QueryPt pt = new QueryPt(mosesModelPath, scores);
        if(pt != null) {
            try {
                BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
                String s = "";
                while ((s = in.readLine()) != null &&  s.length() != 0) {
                    String a = pt.query(s);
                    System.out.print(a);
                }
            }
            catch (IOException e) {
                
            }
        }
    }
}
