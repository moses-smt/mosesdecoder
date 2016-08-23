#!/bin/bash

MOSES_HOME=/opt/moses
GIZA_HOME=${MOSES_HOME}/giza++-v1.0.7
IRSTLM=${MOSES_HOME}/irstlm-5.70.04

function tokenise() {
    local LANG="$1"
    local FILENAME="$2"
    local WORKING_DIR="$3"
    local BASENAME="`basename ${FILENAME}`"

    if [ ! -f ${WORKING_DIR} ]; then
	mkdir -p ${WORKING_DIR}
    fi

    NEW_BASENAME=`echo ${BASENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "tok."; } } }'`

    TOKENISED_FILENAME="${WORKING_DIR}/${NEW_BASENAME}"
    ${MOSES_HOME}/scripts/tokenizer/tokenizer.perl -q -l ${LANG} < ${FILENAME} > ${TOKENISED_FILENAME}
}

function cleanup() {
    local SRC_FILENAME="$1"
    local TGT_FILENAME="$2"
    local SEGMENT_LENGTH="$3"
    SRC_CLEANUP_FILENAME=`echo ${SRC_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "clean."; } } }'`
    TGT_CLEANUP_FILENAME=`echo ${TGT_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "clean."; } } }'`

    truncate -s 0 ${SRC_CLEANUP_FILENAME}
    truncate -s 0 ${TGT_CLEANUP_FILENAME}

    paste -d'\n' ${SRC_FILENAME} ${TGT_FILENAME} | while read SRC_LINE && read TGT_LINE;
    do
      declare -i SRC_NO_WORDS=`echo "${SRC_LINE}" | wc -w`
      declare -i TGT_NO_WORDS=`echo "${TGT_LINE}" | wc -w`
      if [ ${SRC_NO_WORDS} -lt 20 -a ${TGT_NO_WORDS} -lt 20 ]; then
	  echo "${SRC_LINE}" >> ${SRC_CLEANUP_FILENAME}
	  echo "${TGT_LINE}" >> ${TGT_CLEANUP_FILENAME}
      fi
    done
}

function data_split() {
    local SRC_FILENAME="$1"
    local TGT_FILENAME="$2"
    declare -i DEV_SIZE="$3"
    declare -i EVAL_SIZE="$4"

    SRC_TRAIN_FILENAME=`echo ${SRC_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "train."; } } }'`
    TGT_TRAIN_FILENAME=`echo ${TGT_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "train."; } } }'`
    SRC_DEVEL_FILENAME=`echo ${SRC_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "devel."; } } }'`
    TGT_DEVEL_FILENAME=`echo ${TGT_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "devel."; } } }'`
    SRC_EVAL_FILENAME=`echo ${SRC_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "eval."; } } }'`
    TGT_EVAL_FILENAME=`echo ${TGT_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) { printf a[i]; if (i<length(a)) { printf "."; } if (i==length(a)-1) { printf "eval."; } } }'`

    local ALL_FILES=(${SRC_TRAIN_FILENAME} ${TGT_TRAIN_FILENAME} ${SRC_DEVEL_FILENAME} ${TGT_DEVEL_FILENAME} ${SRC_EVAL_FILENAME} ${TGT_EVAL_FILENAME})
    for FN in ${ALL_FILES}
    do
      truncate -s 0 ${FN}
    done

    declare -i DEV_EVAL_SIZE=$(($DEV_SIZE + $EVAL_SIZE))
    declare -i LINE_CNT=1
    paste -d'\n' ${SRC_FILENAME} ${TGT_FILENAME} | while read SRC_LINE && read TGT_LINE;
    do
      if [ ${LINE_CNT} -le ${DEV_EVAL_SIZE} ]; then
	  if [ ${LINE_CNT} -le ${DEV_SIZE} ]; then
	      echo "${SRC_LINE}" >> ${SRC_DEVEL_FILENAME}
	      echo "${TGT_LINE}" >> ${TGT_DEVEL_FILENAME}
	  else
	      echo "${SRC_LINE}" >> ${SRC_EVAL_FILENAME}
	      echo "${TGT_LINE}" >> ${TGT_EVAL_FILENAME}
	  fi
      else
	  echo "${SRC_LINE}" >> ${SRC_TRAIN_FILENAME}
	  echo "${TGT_LINE}" >> ${TGT_TRAIN_FILENAME}
      fi
      LINE_CNT=$(($LINE_CNT + 1))
    done
}

function translation_model_train() {
    declare -l TT_SRC_LANG="$1"
    declare -l TT_TGT_LANG="$2"
    local SRC_FILENAME="`realpath $3`"
    local TGT_FILENAME="`realpath $4`"
    local ALIGNMENT_METHOD="$5"
    local REORDERING_METHOD="$6"
    local WORKING_DIR="$7"

    declare -r SRC_CORPORA_NAME=`echo ${SRC_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i < length(a); i++) { printf a[i]; if (i < length(a) - 1) { printf "."; } } }'`
    declare -r TGT_CORPORA_NAME=`echo ${TGT_FILENAME} | gawk '{split($0, a, "."); for(i = 1; i < length(a); i++) { printf a[i]; if (i < length(a) - 1) { printf "."; } } }'`

    if [ "${SRC_CORPORA_NAME}" != "${TGT_CORPORA_NAME}" ]; then
	echo "Arrrgh"
	exit 1
    fi

    if [ -f ${WORKING_DIR} ]; then
	rm -Rf ${WORKING_DIR} >& /dev/null
    fi
    mkdir -p ${WORKING_DIR}
    WORKING_DIR=`realpath ${WORKING_DIR}`

    declare -r DUMMY_FILE="${WORKING_DIR}/dummy.lm"
    echo "dummy lm file" > ${DUMMY_FILE}

    declare -r LOG_FILE="${WORKING_DIR}/log"

    ${MOSES_HOME}/scripts/training/train-model.perl -root-dir ${WORKING_DIR} -corpus ${SRC_CORPORA_NAME} -f ${TT_SRC_LANG} -e ${TT_TGT_LANG} -alignment ${ALIGNMENT_METHOD} -reordering ${REORDERING_METHOD} -lm 0:5:${DUMMY_FILE}:0 -external-bin-dir ${GIZA_HOME} 2> ${LOG_FILE}

    MOSES_INI_FILE="${WORKING_DIR}/model/moses.ini"
}

function language_model_train() {
    local FILENAME="$1"
    local SMOOTHING_METHOD="$2"
    local WORKING_DIR="$3"

    if [ ! -f ${WORKING_DIR} ]; then
	mkdir -p ${WORKING_DIR}
    fi

    declare -r BASENAME=`basename ${FILENAME}`
    declare -r START_END_OUTPUT_FILENAME=${WORKING_DIR}/`echo ${BASENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) {if(i == 3) { printf "sb."; } else { printf a[i]; if (i < length(a) - 1) { printf "."; } } } }'`
    declare -r LM_FILENAME=${WORKING_DIR}/`echo ${BASENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) {if(i == 3) { printf "lm."; } else { printf a[i]; if (i < length(a) - 1) { printf "."; } } } }'`
    COMPILED_LM_FILENAME=${WORKING_DIR}/`echo ${BASENAME} | gawk '{split($0, a, "."); for(i = 1; i <= length(a); i++) {if(i == 3) { printf "arpa."; } else { printf a[i]; if (i < length(a) - 1) { printf "."; } } } }'`

    export IRSTLM

    ${IRSTLM}/bin/add-start-end.sh < ${FILENAME} > ${START_END_OUTPUT_FILENAME}
    
    declare -r TMP_DIR=`mktemp -dp /tmp`
    ${IRSTLM}/bin/build-lm.sh -i ${START_END_OUTPUT_FILENAME} -t ${TMP_DIR} -p -s ${SMOOTHING_METHOD} -o ${LM_FILENAME}
    if [ -f ${TMP_DIR} ]; then
	rm -Rf ${TMP_DIR} >& /dev/null
    fi

    ${IRSTLM}/bin/compile-lm --text yes ${LM_FILENAME}.gz ${COMPILED_LM_FILENAME}
}

function mert() {
    local MOSES_INI_FILENAME="`realpath $1`"
    local COMPILED_LM_FILENAME="`realpath $2`"
    local EVAL_FILENAME="$3"
    declare -lr _SRC_LANG="$4"
    declare -lr _TGT_LANG="$5"
    declare -ri MODEL_ORDER="$6"
    declare -ri MODEL_TYPE="$7"
    local WORKING_DIR="$8"
    declare -ri MAX_NO_ITERS="$9"

    local INFILENAME=`realpath ${EVAL_FILENAME}`
    INFILENAME=`echo ${INFILENAME} | gawk '{split($0, a, "."); for(i = 1; i < length(a); i++) { printf a[i]; if (i < length(a) - 1) { printf "."; } } }'`

    if [ ! -f ${MOSES_INI_FILENAME} ]; then
	echo "${MOSES_INI_FILENAME} does not exist."
	exit 1
    fi

    if [ -f ${WORKING_DIR} ]; then
	rm -Rf ${WORKING_DIR} >& /dev/null
    fi
    mkdir -p ${WORKING_DIR}

    WORKING_DIR=`realpath ${WORKING_DIR}`
    MERT_INI_FILENAME="${WORKING_DIR}/trained-moses.ini"
    local SED_PROG="/\[lmodel-file\]/,/^[[:space:]]*\$/c\[lmodel-file\]\n${MODEL_TYPE} 0 ${MODEL_ORDER} ${COMPILED_LM_FILENAME}\n"
    eval cat ${MOSES_INI_FILENAME} | sed "${SED_PROG}" > ${MERT_INI_FILENAME}

    ${MOSES_HOME}/scripts/training/mert-moses.pl --maximum-iterations ${MAX_NO_ITERS} --mertdir ${MOSES_HOME}/bin --working-dir ${WORKING_DIR} ${INFILENAME}.${_SRC_LANG} ${INFILENAME}.${_TGT_LANG} ${MOSES_HOME}/bin/moses ${MERT_INI_FILENAME} 2> ${WORKING_DIR}/log
}


if [ $# -lt 4 ]; then
   echo "`basename $0` usage:"
   echo "  `basename $0` src_file tgt_file src_lang tgt_lang"
   echo
   exit 1
fi

declare -r SRC_LANG="$3"
declare -r TGT_LANG="$4"

# Tokenise
tokenise "${SRC_LANG}" "$1" "training/tokeniser"
declare -r SRC_TOKENISED_FILENAME="${TOKENISED_FILENAME}"

tokenise "${TGT_LANG}" "$2" "training/tokeniser"
declare -r TGT_TOKENISED_FILENAME="${TOKENISED_FILENAME}"

echo ${SRC_TOKENISED_FILENAME}
echo ${TGT_TOKENISED_FILENAME}

# Cleanup
cleanup "${SRC_TOKENISED_FILENAME}" "${TGT_TOKENISED_FILENAME}" 20

echo ${SRC_CLEANUP_FILENAME}
echo ${TGT_CLEANUP_FILENAME}

# Data split: src, tgt, dev size, eval size
data_split "${SRC_CLEANUP_FILENAME}" "${TGT_CLEANUP_FILENAME}" 1000 500

echo ${SRC_TRAIN_FILENAME}
echo ${TGT_TRAIN_FILENAME}
echo ${SRC_DEVEL_FILENAME}
echo ${TGT_DEVEL_FILENAME}
echo ${SRC_EVAL_FILENAME}
echo ${TGT_EVAL_FILENAME}

# Train the translation model
translation_model_train "${SRC_LANG}" "${TGT_LANG}" "${SRC_DEVEL_FILENAME}" "${TGT_DEVEL_FILENAME}" "grow-diag-final-and" "msd-bidirectional-fe" "training/model"

declare -r MOSES_TT_INI_FILENAME="${MOSES_INI_FILE}"
echo ${MOSES_TT_INI_FILENAME}

# Language model training
language_model_train "${TGT_TOKENISED_FILENAME}" "improved-kneser-ney" "training/lm"

echo ${COMPILED_LM_FILENAME}

# MERT
mert "${MOSES_TT_INI_FILENAME}" "${COMPILED_LM_FILENAME}" "${SRC_EVAL_FILENAME}" "${SRC_LANG}" "${TGT_LANG}" 3 9 "training/mert" 1

echo ${MERT_INI_FILENAME}
