Vowpal Wabbit for Moses
=======================

This is an attempt to integrate Vowpal Wabbit with Moses as a stateless feature
function.

Implemented classifier features
-------------------------------

* VWFeatureSourceBagOfWords: 
* VWFeatureSourceExternalFeatures:
* VWFeatureSourcePhraseInternal:
* VWFeatureSourceWindow:
* VWFeatureTargetIndicator:
* VWFeatureTargetPhraseInternal:

Configuration
-------------

To use the classifier edit your moses.ini

 [features]
 ...
 VW path=/home/username/vw/classifier1.vw
 VWFeatureSourceBagOfWords
 VWFeatureTargetIndicator
 VWFeatureTargetIndicator
 ...
 
 [weights]
 ...
 VW0= 0.2
 ...
 
If you change the name of the main VW feature, remember to tell the VW classifier
features which classifier they belong to:

 [features]
 ...
 VW name=bart path=/home/username/vw/classifier1.vw 
 VWFeatureSourceBagOfWords used-by=bart
 VWFeatureTargetIndicator used-by=bart
 VWFeatureTargetIndicator used-by=bart
 ...
 
 [weights]
 ...
 bart= 0.2
 ...

You can also use multiple classifiers:

 [features]
 ...
 VW name=bart path=/home/username/vw/classifier1.vw 
 VW path=/home/username/vw/classifier2.vw
 VW path=/home/username/vw/classifier3.vw
 VWFeatureSourceBagOfWords used-by=bart,VW0 
 VWFeatureTargetIndicator used-by=VW1,VW0,bart
 VWFeatureSourceIndicator used-by=bart,VW1
 ...
 
 [weights]
 ...
 bart= 0.2
 VW0= 0.2
 VW1= 0.2
 ...

Training the classifier
-----------------------