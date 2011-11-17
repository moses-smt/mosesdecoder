bin_PROGRAMS = moses_test
moses_test_SOURCES = FeatureVectorTest.cpp \
                     HildrethTest.cpp \
                     MockHypothesis.cpp \
                     MosesTest.cpp \
                     ScoreComponentCollectionTest.cpp \
                     TargetBigramFeatureTest.cpp \
            		     ../mira/Hildreth.cpp

moses_test_CPPFLAGS = -W -Wall -I$(top_srcdir)/moses/src  -I$(top_srcdir)/mira $(BOOST_CPPFLAGS) -DBOOST_TEST_DYN_LINK
moses_test_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(BOOST_THREAD_LIBS) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la -lboost_unit_test_framework
moses_test_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/OnDiskPt/src/libOnDiskPt.a
