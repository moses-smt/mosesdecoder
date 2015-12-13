project(moses)

FILE(GLOB source_moses *.cpp)
FILE(GLOB source_moses_ff FF/*.cpp)
FILE(GLOB source_moses_ff_lexicalReordering FF/LexicalReordering/*.cpp)
FILE(GLOB source_moses_ff_osm FF/OSM-Feature/*.cpp)
FILE(GLOB source_moses_lm LM/*.cpp)
FILE(GLOB source_moses_tm TranslationModel/*.cpp)
FILE(GLOB source_moses_tm_compactPT TranslationModel/CompactPT/*.cpp)
FILE(GLOB source_moses_tm_cky TranslationModel/CKYPlusParser/*.cpp)


add_library(biconcor ${biconcor_source})
