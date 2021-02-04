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

        [DllImport("../../../moses2/x64/Debug/moses2.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetMosesSystem")]
        private static extern int GetMosesSystem(string s, ref IntPtr system);
        [DllImport("../../../moses2/x64/Debug/moses2.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "MosesTranslate")]
        private static extern int MosesTranslate(IntPtr model, int id, string input, StringBuilder output, int output_len);
        [DllImport("../../../moses2/x64/Debug/moses2.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "ReleaseSystem")]
        private static extern int ReleaseSystem(ref IntPtr model);

        static void Main(string[] args)
        {
            string config = "D:\\src\\moses-mstranslator\\test_sentence_with_candidates\\moses_mspt.ini";
            string cand = "aaj din main chaand nikla @@@ aaj ||| आज ||| 0.23034750595193718 $$$ aaj ||| अाज ||| 0.2036812076840512 $$$ aaj ||| एएजे ||| 0.1806033272478164 $$$ aaj ||| आज़ ||| 0.1550204531642581 $$$ din ||| दिन ||| 0.23292194982342979 $$$ din ||| दीन ||| 0.20844420805170855 $$$ din ||| दिं ||| 0.16399885041729953 $$$ din ||| डिन ||| 0.16171304188413235 $$$ chaand ||| चांद ||| 0.2374591084461087 $$$ chaand ||| चाँद ||| 0.217932729237165 $$$ chaand ||| चंद ||| 0.15435859487004985 $$$ chaand ||| चांड ||| 0.15279045900056767 $$$ nikla ||| निकला ||| 0.2727953350543125 $$$ nikla ||| निक्ला ||| 0.15350986400512082 $$$ nikla ||| नीकला ||| 0.1533410959941387 $$$ nikla ||| निकल़ा ||| 0.1475583698921154 $$$ main ||| मैं ||| 0.20812875019912347 $$$ main ||| में ||| 0.2042153102272697 $$$ main ||| मैन ||| 0.1933505532706236 $$$ main ||| मेन ||| 0.18617663610385968";
            IntPtr system = IntPtr.Zero;
            int v = GetMosesSystem(config, ref system);
            StringBuilder output = new StringBuilder();
            var ret = MosesTranslate(system, 1234678, cand, output, 50);
            Console.WriteLine(output);
            ReleaseSystem(ref system);
            Console.ReadLine();
        }
    }
}

