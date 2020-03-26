# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
endif()

# choose the architecture 
set(DEPLOY_PLATFORM x86 CACHE STRING "choose the instruction set architecture")
set_property(CACHE DEPLOY_PLATFORM PROPERTY STRINGS unknown x86 arm64-v8a armeabi-v7a)
IF (${DEPLOY_PLATFORM} STREQUAL "unknown")
	message(FATAL_ERROR "choose the DEPLOY_PLATFORM") 
	return() # This is to stop proceeding further and to stop opencv getting set to the default ANDROID_ABI
ENDIF()


# Set minimum cmake version + cmake settings
cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0048 OLD)

# Specify the project, and set the use of c++ and c
project(cancelthread LANGUAGES CXX C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fPIC -rdynamic -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O2 -std=c++11 ")


include_directories(${PROJECT_SOURCE_DIR})


set(pt-cancel pt-cancel.c)
set(pt-docancel pt-docancel.c)
set(pt-init pt-init.c)
set(pt-setcancelstate pt-setcancelstate.c)
set(pt-setcanceltype pt-setcanceltype.c)
set(pt-test pt-test.c)
set(pt-testcancel pt-testcancel.c)


# Build a shared lib
add_library(cancelthread STATIC ${pt-cancel} ${pt-docancel} ${pt-init} ${pt-setcancelstate} ${pt-setcanceltype} ${pt-test} ${pt-testcancel})

# Build a static lib
# add_library(hts STATIC ${bcf_sr_sort} ${bgzf} ${hts} ${hts_os} ${kfunc} 
# 	${knetfile} ${kstring} ${md5} ${pluging} ${probaln} ${realn} ${regidx} ${region} ${sam} 
# 	${synced_bcf_reader} ${tbx} ${textutils} ${thread_pool} ${vcf} ${vcfutils} ${vcf_sweep} 
# 	${errmod} ${faidx} ${header} ${hfile} ${hfile_net} ${cram_stats} ${cram_codecs} ${cram_decode} 
# 	${cram_encode} ${cram_external} ${cram_index} ${cram_io} ${cram_samtools} ${mFILE} ${open_trace_file} 
# 	${pooled_alloc} ${rANS_static} ${string_alloc} ${multipart} )


IF (${DEPLOY_PLATFORM} STREQUAL "x86")
	target_link_libraries(cancelthread -lz -ldl -lm -lpthread)
ELSE()
	target_link_libraries(cancelthread -lz -ldl -lm)
ENDIF()