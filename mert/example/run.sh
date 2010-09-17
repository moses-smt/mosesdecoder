../extractor  --nbest NBEST --reference REF --ffile FEATSTAT --scfile SCORESTAT --sctype BLEU
../mert --ifile init.opt --scfile SCORESTAT --ffile FEATSTAT -d 15 --verbose 4 -n 5 --sctype BLEU


../extractor --sctype HAMMING,BLEU  --nbest interpolated/tinyNBestAlign --reference interpolated/tinyRef --scconfig refalign:interpolated/tinyRefAlign,source:interpolated/tinySource --ffile interpolated/tinyFeat --scfile interpolated/tinyScoreHB
../mert --ifile interpolated/init.opt --scfile interpolated/tinyScoreHB --ffile interpolated/tinyFeat -d 2 --verbose 4 -n 5 --sctype HAMMING,BLEU --scconfig weights:0.9+0.1


results in orig_BLEU_output

