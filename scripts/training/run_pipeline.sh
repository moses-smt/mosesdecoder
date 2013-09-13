#Script running the training pipeline, replace ~/PATH-to-moses to the location of your mosesdecoder folder

python3 extract_words_dlm.py data/train.en data/train.cs data/alignment > 1_words
python3 make_index_dlm.py --leave-top-lemmas=20 < 1_words > 2_cepts
perl ~/PATH-to-moses/phrase-extract/extract-psd/extract_rich_context_factors.perl train.en.parsed > rich_context
#Usage: extract-dwl dwl-file context-factors-file cept-table extractor-config output-train output-index
touch output-train
touch output-index
~/PATH-to-moses/bin/extract-dwl 1_words rich_context 2_cepts ~/PATH-to-moses/scripts/ems/example/data/dwl-features.ini output-train output-index 
