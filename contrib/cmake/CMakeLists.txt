# This file enables building with cmake on windows  
#
# use cases
#  - generate a visual studio solution and projects, eg: cmake . -G "Visual Studio 15 2017"
#  - or for 64 bit: cmake . -G "Visual Studio 15 2017 Win64"
#  - or set your build options before VS project generation by using the gui, eg:  cmake-gui.exe .
#  - you can set up to build both 32 bit and 64 bit this way:
#       make subfolders '32' and '64' of this cmake folder
#       execute 'cmake-gui  ..' in each.  Select the cmake folder as the 'source code' folder, and the appropriate 32 or 64 folder as 'where to build the binaries' folder
#       for each cmake-gui configuration, choose the suitable 32/64 bit compiler, and corresponding setting for the build_64_bit variable.
#       then Configure, Generate, Open Project (all buttons in the gui) for each.

# check the ./build3rdParty.cmd script in this folder for how to (relatively) easily build the dependencies, typically in a 3rdParty folder outside of the SDK repo.

cmake_minimum_required(VERSION 3.7.2)

#set this to 1 if building 64 bit with eg. cmake . -G "Visual Studio 15 2017 Win64"
#set this to 0 if building 32 bit with eg. cmake . -G "Visual Studio 15 2017"
set (build_64_bit 1 CACHE TYPE BOOL)

#indicate which dependent libraries to use in the build
set (USE_CRYPTOPP 1 CACHE TYPE BOOL)
set (USE_OPENSSL 1 CACHE TYPE BOOL)
set (OPENSSL_IS_BORINGSSL 0 CACHE TYPE BOOL)
set (USE_CURL 1 CACHE TYPE BOOL)
set (USE_SQLITE 1 CACHE TYPE BOOL)
set (USE_MEDIAINFO 1 CACHE TYPE BOOL)
set (USE_FREEIMAGE 1 CACHE TYPE BOOL)
set (USE_SODIUM 0 CACHE TYPE BOOL)
set (ENABLE_SYNC 1 CACHE TYPE BOOL)
set (ENABLE_CHAT 0 CACHE TYPE BOOL)
set (HAVE_FFMPEG 1 CACHE TYPE BOOL)

if (WIN32)
    set (NO_READLINE 1 CACHE TYPE BOOL)
else(WIN32)
    set(NO_READLINE 0)
endif(WIN32)

if (ENABLE_CHAT AND NOT USE_SODIUM)
    message(FATAL_ERROR "ENABLE_CHAT (${ENABLE_CHAT}) requires USE_SODIUM (${USE_SODIUM})")
endif()

if(build_64_bit)
    project (MegaSDK64 LANGUAGES CXX C)
else(build_64_bit)
    project (MegaSDK32 LANGUAGES CXX C)
endif(build_64_bit)

IF(WIN32)
    set(MegaDir "${CMAKE_CURRENT_LIST_DIR}/../..")
    if ("${Mega3rdPartyDir}" STREQUAL "")
        set(Mega3rdPartyDir "${MegaDir}/../3rdparty")
    endif()
ELSE(WIN32)
    set(MegaDir "${CMAKE_CURRENT_LIST_DIR}/../..")
    set(Mega3rdPartyDir "${MegaDir}/../3rdParty/vcpkg/installed/x64-linux")
    include_directories( "${MegaDir}/include" "${Mega3rdPartyDir}/include" )
    link_directories( "${Mega3rdPartyDir}/lib" )
ENDIF(WIN32)

IF(WIN32)
    if(build_64_bit)
        set(vcpkg_dir "${Mega3rdPartyDir}/vcpkg/installed/x64-windows-static-uncheckediterators")
    else(build_64_bit)
        set(vcpkg_dir "${Mega3rdPartyDir}/vcpkg/installed/x86-windows-static-uncheckediterators")
    endif(build_64_bit)
ENDIF(WIN32)

if (NOT CMAKE_BUILD_TYPE)
    message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")
else(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE)

# accurate __cplusplus macro for vc++, selecting c++17 here for windows builds though the MEGA SDK library must build for older c++ standards also
if(WIN32)
add_definitions( -DNOMINMAX -DUSE_CPPTHREAD )  # also don't allow min and max macros from windows.h, use the std functions like on linux/g++
endif()

function(ImportStaticLibrary libName includeDir lib32debug lib32release lib64debug lib64release)
    add_library(${libName} STATIC IMPORTED)
    set_property(TARGET ${libName} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${includeDir})
    if(build_64_bit)
        set_property(TARGET ${libName} PROPERTY IMPORTED_LOCATION_DEBUG ${lib64debug})
        set_property(TARGET ${libName} PROPERTY IMPORTED_LOCATION_RELEASE  ${lib64release})
    else(build_64_bit)
        set_property(TARGET ${libName} PROPERTY IMPORTED_LOCATION_DEBUG ${lib32debug})
        set_property(TARGET ${libName} PROPERTY IMPORTED_LOCATION_RELEASE  ${lib32release})
    endif(build_64_bit)
endfunction(ImportStaticLibrary)

function(ImportVcpkgLibrary libName includeDir)
    # supports alternating debug/release lib names for libraries that have more than one .lib
    # (however it seems the IMPORTED_LLOCAION only supports a single one, so it only supports one debug and one release (for now))
    add_library(${libName} STATIC IMPORTED)
    set_property(TARGET ${libName} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${includeDir})
    set (d 1)
    foreach(libfile ${ARGN})
        if (d EQUAL 1)
            set_property(TARGET ${libName} APPEND PROPERTY IMPORTED_LOCATION_DEBUG ${libfile})
            set(d 2)
        else()
            set_property(TARGET ${libName} APPEND PROPERTY IMPORTED_LOCATION_RELEASE  ${libfile})
            set(d 1)
        endif()
    endforeach(libfile)
endfunction(ImportVcpkgLibrary)

function(ImportHeaderLibrary libName includeDir)
    add_library(${libName} INTERFACE IMPORTED)
    set_property(TARGET ${libName} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${includeDir})
endfunction(ImportHeaderLibrary)

IF(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DCURL_STATICLIB -DCARES_STATICLIB -DWIN32_LEAN_AND_MEAN -DUNICODE -DSODIUM_STATIC -DPCRE_STATICWIN32 -D_CONSOLE)
    SET(Mega_PlatformSpecificIncludes ${MegaDir}/include/mega/$<IF:${USE_CURL},wincurl,win32>)
    SET(Mega_PlatformSpecificLibs ws2_32 winhttp Shlwapi Secur32.lib)

    SET(Mega_PlatformSpecificFiles
    ${MegaDir}/src/win32/waiter.cpp
    ${MegaDir}/src/thread/cppthread.cpp
    )


ELSE(WIN32)

    add_definitions(-DUSE_PTHREAD )
    SET(Mega_PlatformSpecificFiles ${MegaDir}/src/posix/console.cpp ${MegaDir}/src/posix/consolewaiter.cpp ${MegaDir}/src/posix/waiter.cpp ${MegaDir}/src/thread/posixthread.cpp )
    SET(Mega_PlatformSpecificIncludes ${MegaDir}/include/mega/posix)
    SET(Mega_PlatformSpecificLibs crypto dl)

ENDIF(WIN32)

include(CheckIncludeFile)
include(CheckFunctionExists)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(uv.h HAVE_LIBUV)
check_function_exists(aio_write, HAVE_AIO_RT)
check_function_exists(fdopendir, HAVE_FDOPENDIR)

configure_file ("${MegaDir}/include/mega/config.h.in.cmake" "${CMAKE_BINARY_DIR}/include/mega/config.h" )



SET(Mega_CryptoFiles ${MegaDir}/src/crypto/cryptopp.cpp ${MegaDir}/src/crypto/sodium.cpp)
SET(Mega_DbFiles ${MegaDir}/src/db/sqlite.cpp ${MegaDir}/src/db/sqlite.cpp )
SET(Mega_GfxFiles ${MegaDir}/src/gfx/external.cpp ${MegaDir}/src/gfx/freeimage.cpp ) 

add_library(Mega STATIC
            ${MegaDir}/src/attrmap.cpp 
            ${MegaDir}/src/backofftimer.cpp 
            ${MegaDir}/src/base64.cpp 
            ${MegaDir}/src/command.cpp 
            ${MegaDir}/src/commands.cpp 
            ${MegaDir}/src/db.cpp 
            ${MegaDir}/src/file.cpp 
            ${MegaDir}/src/fileattributefetch.cpp 
            ${MegaDir}/src/filefingerprint.cpp 
            ${MegaDir}/src/filesystem.cpp 
            ${MegaDir}/src/gfx.cpp 
            ${MegaDir}/src/http.cpp 
            ${MegaDir}/src/json.cpp 
            ${MegaDir}/src/logging.cpp 
            ${MegaDir}/src/mediafileattribute.cpp 
            ${MegaDir}/src/mega_ccronexpr.cpp 
            ${MegaDir}/src/mega_http_parser.cpp 
            ${MegaDir}/src/mega_utf8proc.cpp 
            ${MegaDir}/src/mega_zxcvbn.cpp
            ${MegaDir}/src/megaclient.cpp 
            ${MegaDir}/src/node.cpp 
            ${MegaDir}/src/pendingcontactrequest.cpp 
            ${MegaDir}/src/proxy.cpp 
            ${MegaDir}/src/pubkeyaction.cpp 
            ${MegaDir}/src/request.cpp
            ${MegaDir}/src/serialize64.cpp 
            ${MegaDir}/src/share.cpp 
            ${MegaDir}/src/sharenodekeys.cpp 
            ${MegaDir}/src/sync.cpp 
            ${MegaDir}/src/transfer.cpp 
            ${MegaDir}/src/transferslot.cpp 
            ${MegaDir}/src/treeproc.cpp 
            ${MegaDir}/src/user.cpp 
            ${MegaDir}/src/useralerts.cpp 
            ${MegaDir}/src/utils.cpp 
            ${MegaDir}/src/waiterbase.cpp 
            ${Mega_PlatformSpecificFiles} ${Mega_CryptoFiles} ${Mega_DbFiles} ${Mega_GfxFiles}  )

target_include_directories(Mega PRIVATE ${MegaDir}/include ${Mega_PlatformSpecificIncludes})
target_include_directories(Mega PUBLIC ${MegaDir}/include ${Mega_PlatformSpecificIncludes})
target_link_libraries(Mega PUBLIC
                $<${USE_CRYPTOPP}:cryptopp> 
                $<${USE_SODIUM}:sodium> 
                $<${USE_OPENSSL}:ssl> $<${USE_OPENSSL}:crypto> 
                $<${USE_CURL}:curl>  $<${USE_CURL}:cares> 
                $<${USE_SQLITE}:sqlite3> 
                $<${USE_MEDIAINFO}:mediainfo> $<${USE_MEDIAINFO}:zen> 
                $<${USE_FREEIMAGE}:freeimage> $<${USE_FREEIMAGE}:freeimage_IlmImf> $<${USE_FREEIMAGE}:freeimage_IlmImfUtil> $<${USE_FREEIMAGE}:freeimage_IlmThread> $<${USE_FREEIMAGE}:freeimage_Iex> $<${USE_FREEIMAGE}:freeimage_IexMath>
                                              $<${USE_FREEIMAGE}:freeimage_jpeg> $<${USE_FREEIMAGE}:freeimage_turbojpeg> $<${USE_FREEIMAGE}:freeimage_jpegxr> $<${USE_FREEIMAGE}:freeimage_jxrglue> $<${USE_FREEIMAGE}:freeimage_openjp2>
                                              $<${USE_FREEIMAGE}:freeimage_lzma>  $<${USE_FREEIMAGE}:freeimage_lcms2>  $<${USE_FREEIMAGE}:freeimage_raw>  
                                              $<${USE_FREEIMAGE}:freeimage_tiff>  $<${USE_FREEIMAGE}:freeimage_tiffxx>  
                                              $<${USE_FREEIMAGE}:freeimage_jasper>  $<${USE_FREEIMAGE}:freeimage_libpng>  $<${USE_FREEIMAGE}:freeimage_half>  
                                              $<${USE_FREEIMAGE}:freeimage_webp> $<${USE_FREEIMAGE}:freeimage_webpdecoder>  $<${USE_FREEIMAGE}:freeimage_webpdemux>  $<${USE_FREEIMAGE}:freeimage_webpmux> 
                                              $<${USE_FREEIMAGE}:freeimage_swscale>  $<${USE_FREEIMAGE}:freeimage_swresample>  
                $<${HAVE_FFMPEG}:ffmpeg_avformat> $<${HAVE_FFMPEG}:ffmpeg_avcodec> $<${HAVE_FFMPEG}:ffmpeg_avutil> $<${HAVE_FFMPEG}:ffmpeg_avfilter> $<${HAVE_FFMPEG}:ffmpeg_avdevice> $<${HAVE_FFMPEG}:ffmpeg_avdevice> 
                ${Mega_PlatformSpecificLibs})
target_compile_definitions(Mega PUBLIC 
                $<${USE_MEDIAINFO}:USE_MEDIAINFO> 
                $<${USE_SQLITE}:USE_SQLITE> 
                $<${USE_CRYPTOPP}:USE_CRYPTOPP> 
                $<${USE_OPENSSL}:USE_OPENSSL> 
                $<${USE_CURL}:USE_CURL> 
                $<${USE_SODIUM}:USE_SODIUM>
                $<${ENABLE_SYNC}:ENABLE_SYNC> 
                $<${ENABLE_CHAT}:ENABLE_CHAT> 
                $<${NO_READLINE}:NO_READLINE>
                $<${USE_FREEIMAGE}:USE_FREEIMAGE> 
                $<${HAVE_FFMPEG}:HAVE_FFMPEG> )

if(WIN32)
target_link_libraries(Mega PUBLIC crypt32.lib)
endif(WIN32)

#test apps
add_executable(test_sdk             ${MegaDir}/tests/sdk_test.cpp 
                                    ${MegaDir}/tests/sdktests.cpp)
add_executable(test_misc            ${MegaDir}/tests/tests.cpp 
                                    ${MegaDir}/tests/paycrypt_test.cpp 
                                    ${MegaDir}/tests/crypto_test.cpp)
add_executable(test_purge_account   ${MegaDir}/tests/purge_account.cpp)
add_executable(test_sync            ${MegaDir}/tests/synctests.cpp)

target_compile_definitions(test_sdk PRIVATE _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
target_compile_definitions(test_misc PRIVATE _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
target_compile_definitions(test_purge_account PRIVATE _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
target_compile_definitions(test_sync PRIVATE _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
target_link_libraries(test_sdk gtest Mega )
target_link_libraries(test_misc gtest Mega )
target_link_libraries(test_purge_account gtest Mega )
target_link_libraries(test_sync gtest Mega )

#test apps need this file or tests fail
configure_file("${MegaDir}/logo.png" logo.png COPYONLY)

# actual apps

add_executable(megacli ${MegaDir}/examples/megacli.cpp)
target_link_libraries(megacli Mega)
if (NOT NO_READLINE)
    target_link_libraries(megacli readline)
endif (NOT NO_READLINE)

add_executable(megasimplesync ${MegaDir}/examples/megasimplesync.cpp)
target_link_libraries(megasimplesync Mega )

if(WIN32)
add_executable(testmega "${MegaDir}/examples/win32/testmega/main.cpp")
target_link_libraries(testmega Mega )
endif(WIN32)

#enable_testing()
#add_test(NAME SdkTestStreaming COMMAND test_sdk "--gtest_filter=\"*Streaming*\"")
#add_test(NAME SdkTestAll COMMAND test_sdk )

