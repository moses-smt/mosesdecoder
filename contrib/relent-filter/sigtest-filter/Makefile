SALMDIR=/Users/hieuhoang/workspace/salm
FLAVOR?=o64
INC=-I$(SALMDIR)/Src/Shared -I$(SALMDIR)/Src/SuffixArrayApplications -I$(SALMDIR)/Src/SuffixArrayApplications/SuffixArraySearch
OBJS=$(SALMDIR)/Distribution/Linux/Objs/Search/_SuffixArrayApplicationBase.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Search/_SuffixArraySearchApplicationBase.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Shared/_String.$(FLAVOR) $(SALMDIR)/Distribution/Linux/Objs/Shared/_IDVocabulary.$(FLAVOR)

all: filter-pt

filter-pt: filter-pt.cpp
	./check-install $(SALMDIR)
	$(CXX) -O6 $(INC) $(OBJS) -o filter-pt filter-pt.cpp
