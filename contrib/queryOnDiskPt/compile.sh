SRI=/Users/hieuhoang/workspace/srilm
IRST=/Users/hieuhoang/workspace/irstlm/trunk

g++ -o queryOnDiskPt queryOnDiskPt.cpp ../../moses/src/PhraseDictionary.cpp -I../../moses/src/ -I../../ -L../../dist/lib/ -I../../OnDiskPt -lmert_lib -ldynsa -lz -lmoses_internal -lOnDiskPt -lLM -lkenlm -lkenutil -lRuleTable -lCYKPlusParser -lScope3Parser -L$SRI/lib/macosx/ -ldstruct -lflm -llattice -lmisc -loolm -L/opt/local/lib -lboost_thread-mt -L$IRST/lib -lirstlm


