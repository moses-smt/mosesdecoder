[11/09/2010]
MOSES FOR MERE MORTALS
======================
Moses for Mere Mortals (MMM) has been tested with Ubuntu 10.04 LTS and the Moses version published on August 13, 2010 and updated on August 14, 2010 (http://sourceforge.net/projects/mosesdecoder/files/mosesdecoder/2010-08-13/moses-2010-08-13.tgz/download).

***PURPOSES***:
===============

1) MOSES INSTALLATION WITH A SINGLE COMMAND
-------------------------------------------
If you aren't used to compiling Linux programs (both Moses and the packages upon which it depends), you'll love this!

2) MOSES VERY SIMPLE DEMO
-------------------------
MMM is meant to quickly allow you to get results with Moses. You can place MMM wherever you prefer on your hard disk and then call, with a single command, each of its several scripts (their version number is omitted here): 
a) create (in order to compile Moses and the packages it uses with a single command); 
b) make-test-files; 
c) train; 
d) translate; 
e) score the translation(s) you got; and 
f) transfer trained corpora between users or to other places of your disk. 

MMM uses non-factored training, a type of training that in our experience already produces good results in a significant number of language pairs, and mainly with non-morphologically rich languages or with language pairs in which the target language is not morphologically rich. A Quick-Start-Guide should help you to quickly get the feel of it and start getting results. 

It comes with a small demo corpus, too small to do justice to the quality that Moses can achieve, but sufficient for you to get a general overview of SMT and an idea of how useful Moses can be to your work, if you are strting to use it. 
 
3) PROTOTYPE OF A REAL WORLD TRANSLATION CHAIN
----------------------------------------------
MMM enables you to use very large corpora and is being used for that purpose (translation for real translators) in our working environment. It was made having in mind that, in order to get the best results, corpora should be based on personal (ou group's) files and that many translators use translation memories. Therefore, we have coupled it with two Windows add-ins that enable you to convert your TMX files into Moses corpora and also allow you to convert the Moses translations into TMX files that translators can use with a translation memory tool. 

4) WAY OF STARTING LEARNING MOSES AND MACHINE TRANSLATION
---------------------------------------------------------
MMM also comes with a very detailed Help-Tutorial (in its docs subdirectory). It therefore should ease the learning path for true beginners. The scripts code isn't particularly elegant, but most of it should be easily understandable even by beginners (if they read the Moses documentation, that is!). What's more, it does work!

MMM was designed to be very easy and immediately feasible to use and that's indeed why it was made for mere mortals and called as such.

***SOME CHARACTERISTICS***:
===========================
 1) Compiles all the packages used by these scripts with a single instruction;
 2) Removes control characters from the input files (these can crash a training);
 3) Extracts from the corpus files 2 test files by pseudorandomly selecting non-consecutive segments that are erased from the corpus files;
 4) A new training does not interfere with the files of a previous training;
 5) A new training reuses as much as possible the files created in previous trainings (thus saving time);
 6) Detects inversions of corpora (e.g., from en-pt to pt-en), allowing a much quicker training than that of the original language pair (also checks that the inverse training is correct);
 7) Stops with an informative message if any of the phases of training (language model building, recaser training, corpus training, memory-mapping, tuning or training test) doesn't produce the expected results;
 8) Can limit the duration of tuning;
 9) Generates the BLEU and NIST scores of a translation or of a set of translations placed in a single directory (either for each whole document or for each segment of it);
10) Allows you to transfer your trainings to someone else's computer or to another Moses installation in the same computer;
11) All the mkcls, GIZA and MGIZA parameters can be controlled through parameters of the train script;
12) Selected parameters of the Moses scripts and the Moses decoder can be controlled with the train and translate scripts.




