SALMDIR=/chomes/redpony/salm
FLAVOR=o32
INC=-I$(SALMDIR)/Src/Shared -I$(SALMDIR)/Src/SuffixArrayApplications -I$(SALMDIR)/Src/SuffixArrayApplications/SuffixArraySearch
OBJS=$(SALMDIR)/Distribution/Linux/Objs/Search/_SuffixArrayApplicationBase.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Search/_SuffixArraySearchApplicationBase.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Shared/_String.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Shared/_IDVocabulary.$(FLAVOR)

all: filter-pt score-paraphrases

filter-pt: filter-pt.cpp
	./check-install $(SALMDIR)
	g++ -O6 $(INC) $(OBJS) -o filter-pt filter-pt.cpp

score-paraphrases: score-paraphrases.cpp
	./check-install $(SALMDIR)
	g++ -O0 -g $(INC) $(OBJS) -o score-paraphrases score-paraphrases.cpp

