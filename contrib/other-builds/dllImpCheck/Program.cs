using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace dllImpCheck
{
    class Program
    {
        
        [DllImport("D:/moses-mstranslator/contrib/other-builds/moses2/x64/Debug/moses2.dll", EntryPoint = "getEngineVersion1")]
        private static extern int getEngineVersion1();
        [DllImport("D:/moses-mstranslator/contrib/other-builds/moses2/x64/Debug/moses2.dll", EntryPoint = "CreateMosesSystem")]
        private static extern IntPtr CreateMosesSystem(string s);
        [DllImport("D:/moses-mstranslator/contrib/other-builds/moses2/x64/Debug/moses2.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetMosesSystem")]
        private static extern int GetMosesSystem(string s,ref IntPtr system);
        [DllImport("D:/moses-mstranslator/contrib/other-builds/moses2/x64/Debug/moses2.dll", EntryPoint = "MosesTranslate")]
        private static extern int MosesTranslate(IntPtr model,int id,string input,StringBuilder output,int output_len);
        [DllImport("D:/moses-mstranslator/contrib/other-builds/moses2/x64/Debug/moses2.dll", EntryPoint = "ReleaseSystem")]
        private static extern void ReleaseSystem(IntPtr model);
        static void Main(string[] args)
        {
            string a = "D:/moses-mstranslator/test_sentence_with_candidates/moses_mspt.ini";
            string cand = "फ ो ट ो ं @@@ ट ||| a ||| 0.5338410658500136 $$$ ट ||| c ||| 0.10587171128910133 $$$ ट ||| m ||| 0.7056508746775306 $$$ ं ||| l ||| 0.29237797398236876 $$$ ं ||| o ||| 0.4026301817948226 $$$ ं ||| r ||| 0.20594041196734436 $$$ फ ||| c ||| 0.46792456587433573 $$$ फ ||| g ||| 0.43855815762641204 $$$ फ ||| x ||| 0.7077570324853759 $$$ ो ||| h ||| 0.9869239425073358 $$$ ो ||| i ||| 0.6660016809625412 $$$ ो ||| h ||| 0.8425506301302961";
            IntPtr system =  new IntPtr(0);
            int v = GetMosesSystem(a,ref system);
            StringBuilder output = new StringBuilder();
            int error_code = MosesTranslate(system,1234678,cand,output,50);
            Console.WriteLine(output);

        }
    }
}

