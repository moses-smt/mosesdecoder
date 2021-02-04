using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Moses;
namespace MosesWrapperTest
{
    class Program { 
    
        static void Main(string[] args)
        {
            Moses2Wrapper e = new Moses2Wrapper("D:/moses-mstranslator/test_sentence_with_candidates/moses_mspt.ini");
            string mystring = e.Translate("फ ो ट ो ं @@@ ट ||| a ||| 0.5338410658500136 $$$ ट ||| c ||| 0.10587171128910133 $$$ ट ||| m ||| 0.7056508746775306 $$$ ं ||| l ||| 0.29237797398236876 $$$ ं ||| o ||| 0.4026301817948226 $$$ ं ||| r ||| 0.20594041196734436 $$$ फ ||| c ||| 0.46792456587433573 $$$ फ ||| g ||| 0.43855815762641204 $$$ फ ||| x ||| 0.7077570324853759 $$$ ो ||| h ||| 0.9869239425073358 $$$ ो ||| i ||| 0.6660016809625412 $$$ ो ||| h ||| 0.8425506301302961", 123456789);
            Console.WriteLine(mystring);
            return;
        }
    }
}
