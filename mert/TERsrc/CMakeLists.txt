PROJECT(tercpp)


# SET(sgmlDocument_LIB_SRCS
#   sgmlDocument.cpp
# )
SET(tools_LIB_SRCS
   tools.cpp
)
SET(xmlStructure_LIB_SRCS
   xmlStructure.cpp
)
SET(alignmentStruct_LIB_SRCS
   alignmentStruct.cpp
)
SET(terAlignment_LIB_SRCS
   terAlignment.cpp
)
SET(stringHasher_LIB_SRCS
   stringHasher.cpp
)
SET(stringInfosHasher_LIB_SRCS
   stringInfosHasher.cpp
)
SET(infosHasher_LIB_SRCS
   infosHasher.cpp
)
SET(hashMap_LIB_SRCS
   hashMap.cpp
)
SET(tercalc_LIB_SRCS
   tercalc.cpp
)
SET(hashMapInfos_LIB_SRCS
   hashMapInfos.cpp
)
SET(hashMapStringInfos_LIB_SRCS
   hashMapStringInfos.cpp
)
SET(terShift_LIB_SRCS
   terShift.cpp
)
SET(documentStructure_LIB_SRCS
   documentStructure.cpp
)
SET(segmentStructure_LIB_SRCS
   segmentStructure.cpp
)
SET(tinyxml_LIB_SRCS
   tinystr.cpp  tinyxml.cpp  tinyxmlerror.cpp  tinyxmlparser.cpp
)


# add_library(sgmlDocument SHARED ${sgmlDocument_LIB_SRCS})
add_library(tools  SHARED ${tools_LIB_SRCS})
add_library(documentStructure SHARED ${documentStructure_LIB_SRCS})
add_library(segmentStructure SHARED ${segmentStructure_LIB_SRCS})
add_library(xmlStructure SHARED ${xmlStructure_LIB_SRCS})
add_library(alignmentStruct  SHARED ${tools_LIB_SRCS})
add_library(terAlignment  SHARED ${terAlignment_LIB_SRCS})
add_library(stringHasher  SHARED ${stringHasher_LIB_SRCS})
add_library(stringInfosHasher  SHARED ${stringInfosHasher_LIB_SRCS})
add_library(infosHasher  SHARED ${infosHasher_LIB_SRCS})
add_library(hashMap SHARED  ${hashMap_LIB_SRCS})
add_library(tercalc SHARED  ${tercalc_LIB_SRCS})
add_library(hashMapInfos SHARED  ${hashMapInfos_LIB_SRCS})
add_library(hashMapStringInfos SHARED  ${hashMapStringInfos_LIB_SRCS})
add_library(terShift SHARED  ${terShift_LIB_SRCS})
add_library(tinyxml  SHARED ${tinyxml_LIB_SRCS})







target_link_libraries(documentStructure tools segmentStructure)
target_link_libraries(alignmentStruct tools)
target_link_libraries(hashMap stringHasher tools)
target_link_libraries(hashMapInfos infosHasher tools )
target_link_libraries(hashMapStringInfos stringInfosHasher tools )
target_link_libraries(tercalc hashMap hashMapInfos terAlignment terShift tools alignmentStruct hashMapStringInfos boost_date_time boost_regex boost_serialization tinyxml)

set_target_properties(tinyxml PROPERTIES VERSION 0.2 SOVERSION 0)
set_target_properties(segmentStructure PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(xmlStructure PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(alignmentStruct PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(tools PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(terShift PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(terAlignment PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(stringHasher PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(infosHasher PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(stringInfosHasher PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(hashMap PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(hashMapInfos PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(hashMapStringInfos PROPERTIES VERSION 0.1 SOVERSION 0)
set_target_properties(tercalc PROPERTIES VERSION 0.1 SOVERSION 0)
# set_target_properties(sgmlDocument PROPERTIES VERSION 0.1 SOVERSION 0)



install(TARGETS tercalc stringHasher stringInfosHasher hashMapStringInfos hashMap infosHasher hashMapInfos terAlignment terShift tools alignmentStruct xmlStructure segmentStructure documentStructure tinyxml DESTINATION lib)


install(FILES tercalc.h stringHasher.h hashMap.h hashMapInfos.h infosHasher.h terAlignment.h terShift.h tools.h alignmentStruct.h bestShiftStruct.h stringInfosHasher.h hashMapStringInfos.h xmlStructure.h segmentStructure.h documentStructure.h tinystr.h  tinyxml.h DESTINATION include)
 

