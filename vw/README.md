Vowpal Wabbit for Moses
=======================

This is an attempt to integrate Vowpal Wabbit with Moses as a stateless feature
function.

Implemented classifier features
-------------------------------

* `VWFeatureSourceBagOfWords`: This creates a feature of form bow^token for every
source sentence token.
* `VWFeatureSourceExternalFeatures column=0`: when used with -inputtype 5 (`TabbedSentence`) this can be used to supply additional feature to VW. The input is a tab-separated file, the first column is the usual input sentence, all other columns can be used for meta-data. Parameter column=0 counts beginning with the first column that is not the input sentence.  
* `VWFeatureSourceIndicator`: Ass a feature for the whole source phrase.
* `VWFeatureSourcePhraseInternal`: Adds a separate feature for every word of the source phrase.
* `VWFeatureSourceWindow size=3`: Adds source words in a window of size 3 before and after the source phrase as features. These does not overlap with `VWFeatureSourcePhraseInternal`.
* `VWFeatureTargetIndicator`: Adds a feature for the whole target phrase.
* `VWFeatureTargetPhraseInternal`: Adds a separate feature for every word of the target phrase.

Configuration
-------------

To use the classifier edit your moses.ini

    [features]
    ...
    VW path=/home/username/vw/classifier1.vw
    VWFeatureSourceBagOfWords
    VWFeatureTargetIndicator
    VWFeatureSourceIndicator
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
    VWFeatureSourceIndicator used-by=bart
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
TODO
