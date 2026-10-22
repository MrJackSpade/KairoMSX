# The libraries openMSX needs, built from the pinned snapshots in third_party/: zlib, libpng, Tcl
# and SDL2. SDL2 is built without its video, audio and input drivers: openMSX uses it for event
# types, key names, surfaces and timing, while KairoMSX supplies video, sound and input itself.

get_filename_component(THIRD_PARTY "${CMAKE_CURRENT_LIST_DIR}/../third_party" ABSOLUTE)

# zlib
set(ZLIB_DIR "${THIRD_PARTY}/zlib")
add_library(kairo_zlib STATIC
    "${ZLIB_DIR}/adler32.c" "${ZLIB_DIR}/compress.c" "${ZLIB_DIR}/crc32.c" "${ZLIB_DIR}/deflate.c"
    "${ZLIB_DIR}/gzclose.c" "${ZLIB_DIR}/gzlib.c" "${ZLIB_DIR}/gzread.c" "${ZLIB_DIR}/gzwrite.c"
    "${ZLIB_DIR}/infback.c" "${ZLIB_DIR}/inffast.c" "${ZLIB_DIR}/inflate.c" "${ZLIB_DIR}/inftrees.c"
    "${ZLIB_DIR}/trees.c" "${ZLIB_DIR}/uncompr.c" "${ZLIB_DIR}/zutil.c")
target_include_directories(kairo_zlib PUBLIC "${ZLIB_DIR}")
if(NOT WIN32)
    target_compile_definitions(kairo_zlib PRIVATE HAVE_UNISTD_H)
endif()

# libpng, with its stock configuration; only screenshots and images use it, so without NEON code.
set(PNG_DIR "${THIRD_PARTY}/libpng")
set(PNG_CONF "${CMAKE_CURRENT_BINARY_DIR}/libpng")
configure_file("${PNG_DIR}/scripts/pnglibconf.h.prebuilt" "${PNG_CONF}/pnglibconf.h" COPYONLY)
add_library(kairo_png STATIC
    "${PNG_DIR}/png.c" "${PNG_DIR}/pngerror.c" "${PNG_DIR}/pngget.c" "${PNG_DIR}/pngmem.c"
    "${PNG_DIR}/pngpread.c" "${PNG_DIR}/pngread.c" "${PNG_DIR}/pngrio.c" "${PNG_DIR}/pngrtran.c"
    "${PNG_DIR}/pngrutil.c" "${PNG_DIR}/pngset.c" "${PNG_DIR}/pngtrans.c" "${PNG_DIR}/pngwio.c"
    "${PNG_DIR}/pngwrite.c" "${PNG_DIR}/pngwtran.c" "${PNG_DIR}/pngwutil.c")
target_include_directories(kairo_png PUBLIC "${PNG_DIR}" "${PNG_CONF}")
target_compile_definitions(kairo_png PRIVATE PNG_ARM_NEON_OPT=0)
target_link_libraries(kairo_png PUBLIC kairo_zlib)

# Tcl 8.6, configured as its own configure script configures it for each target, with threads,
# statically, without extension loading (tclLoadNone) and with KairoMSX's zlib. The definitions are
# configure's (unix/configure --host=aarch64-linux-android --enable-threads --disable-shared
# --disable-load; win/configure --host=x86_64-w64-mingw32 --enable-threads --disable-shared
# --enable-64bit), except the C library replacements configure picks when it cannot run test
# programs while cross-compiling, which Bionic does not need.
set(TCL_DIR "${THIRD_PARTY}/tcl")
set(TCL_GENERIC
    regcomp regexec regfree regerror tclAlloc tclAssembly tclAsync tclBasic tclBinary tclCkalloc
    tclClock tclCmdAH tclCmdIL tclCmdMZ tclCompCmds tclCompCmdsGR tclCompCmdsSZ tclCompExpr
    tclCompile tclConfig tclDate tclDictObj tclDisassemble tclEncoding tclEnsemble tclEnv tclEvent
    tclExecute tclFCmd tclFileName tclGet tclHash tclHistory tclIndexObj tclInterp tclIO tclIOCmd
    tclIORChan tclIORTrans tclIOGT tclIOSock tclIOUtil tclLink tclListObj tclLiteral tclLoad
    tclMain tclNamesp tclNotify tclObj tclOptimize tclPanic tclParse tclPathObj tclPipe tclPkg
    tclPkgConfig tclPosixStr tclPreserve tclProc tclRegexp tclResolve tclResult tclScan
    tclStringObj tclStrToD tclThread tclThreadAlloc tclThreadJoin tclThreadStorage tclStubInit
    tclTimer tclTrace tclUtf tclUtil tclVar tclZlib tclTomMathInterface
    tclOO tclOOBasic tclOOCall tclOODefineCmds tclOOInfo tclOOMethod tclOOStubInit tclLoadNone)
set(TCL_TOMMATH
    bn_s_mp_reverse bn_s_mp_mul_digs_fast bn_s_mp_sqr_fast bn_mp_add bn_mp_and bn_mp_add_d
    bn_mp_clamp bn_mp_clear bn_mp_clear_multi bn_mp_cmp bn_mp_cmp_d bn_mp_cmp_mag bn_mp_cnt_lsb
    bn_mp_copy bn_mp_count_bits bn_mp_div bn_mp_div_d bn_mp_div_2 bn_mp_div_2d bn_s_mp_div_3
    bn_mp_exch bn_mp_expt_n bn_mp_grow bn_mp_init bn_mp_init_copy bn_mp_init_multi bn_mp_init_set
    bn_mp_init_size bn_s_mp_karatsuba_mul bn_s_mp_karatsuba_sqr bn_s_mp_balance_mul bn_mp_lshd
    bn_mp_mod bn_mp_mod_2d bn_mp_mul bn_mp_mul_2 bn_mp_mul_2d bn_mp_mul_d bn_mp_neg bn_mp_or
    bn_mp_pack bn_mp_pack_count bn_mp_radix_size bn_mp_radix_smap bn_mp_read_radix bn_mp_rshd
    bn_mp_shrink bn_mp_set bn_mp_sqr bn_mp_sqrt bn_mp_sub bn_mp_sub_d bn_mp_signed_rsh
    bn_mp_to_ubin bn_mp_unpack bn_s_mp_toom_mul bn_s_mp_toom_sqr bn_mp_to_radix bn_mp_ubin_size
    bn_mp_xor bn_mp_zero bn_s_mp_add bn_s_mp_mul_digs bn_s_mp_sqr bn_s_mp_sub)
set(TCL_SOURCES)
foreach(name ${TCL_GENERIC})
    list(APPEND TCL_SOURCES "${TCL_DIR}/generic/${name}.c")
endforeach()
foreach(name ${TCL_TOMMATH})
    list(APPEND TCL_SOURCES "${TCL_DIR}/libtommath/${name}.c")
endforeach()
if(WIN32)
    foreach(name tclWin32Dll tclWinChan tclWinConsole tclWinSerial tclWinError tclWinFCmd tclWinFile
                 tclWinInit tclWinLoad tclWinNotify tclWinPipe tclWinSock tclWinThrd tclWinTime)
        list(APPEND TCL_SOURCES "${TCL_DIR}/win/${name}.c")
    endforeach()
    list(REMOVE_ITEM TCL_SOURCES "${TCL_DIR}/generic/tclLoadNone.c")
    set(TCL_PLATFORM_DIR "${TCL_DIR}/win")
    set(TCL_PLATFORM_DEFINITIONS HAVE_CPUID=1 TCL_CFGVAL_ENCODING="cp1252" MODULE_SCOPE=extern
        TCL_CFG_DO64BIT=1 HAVE_NO_SEH=1 HAVE_INTRIN_H=1 HAVE_CPUID_H=1 HAVE_WSPIAPI_H=1)
else()
    foreach(name tclUnixChan tclUnixEvent tclUnixFCmd tclUnixFile tclUnixPipe tclUnixSock
                 tclUnixTime tclUnixInit tclUnixThrd tclUnixCompat tclUnixNotfy)
        list(APPEND TCL_SOURCES "${TCL_DIR}/unix/${name}.c")
    endforeach()
    set(TCL_PLATFORM_DIR "${TCL_DIR}/unix")
    set(TCL_PLATFORM_DEFINITIONS
        NO_VALUES_H=1 HAVE_SYS_PARAM_H=1 _REENTRANT=1 _THREAD_SAFE=1 HAVE_PTHREAD_ATTR_SETSTACKSIZE=1
        HAVE_PTHREAD_ATFORK=1 TCL_CFGVAL_ENCODING="iso8859-1"
        "MODULE_SCOPE=extern __attribute__((__visibility__(\"hidden\")))" HAVE_HIDDEN=1 HAVE_VFORK=1
        TCL_SHLIB_EXT="" TCL_WIDE_INT_IS_LONG=1 HAVE_GETCWD=1 HAVE_MKSTEMP=1 HAVE_OPENDIR=1
        HAVE_STRTOL=1 HAVE_WAITPID=1 NO_GETWD=1 NO_WAIT3=1 HAVE_GETNAMEINFO=1 HAVE_GETADDRINFO=1
        HAVE_FREEADDRINFO=1 HAVE_GAI_STRERROR=1 HAVE_STRUCT_ADDRINFO=1 HAVE_STRUCT_IN6_ADDR=1
        HAVE_STRUCT_SOCKADDR_IN6=1 HAVE_STRUCT_SOCKADDR_STORAGE=1 HAVE_GETPWUID_R_5=1
        HAVE_GETPWUID_R=1 HAVE_GETPWNAM_R_5=1 HAVE_GETPWNAM_R=1 HAVE_GETGRGID_R_5=1
        HAVE_GETGRGID_R=1 HAVE_GETGRNAM_R_5=1 HAVE_GETGRNAM_R=1 HAVE_DECL_GETHOSTBYNAME_R=1
        HAVE_GETHOSTBYNAME_R_6=1 HAVE_GETHOSTBYNAME_R=1 HAVE_DECL_GETHOSTBYADDR_R=1
        HAVE_GETHOSTBYADDR_R_8=1 HAVE_GETHOSTBYADDR_R=1 HAVE_TERMIOS_H=1 HAVE_SYS_IOCTL_H=1
        HAVE_SYS_SELECT_H=1 HAVE_SYS_TIME_H=1 TIME_WITH_SYS_TIME=1 HAVE_GMTIME_R=1
        HAVE_LOCALTIME_R=1 HAVE_MKTIME=1 HAVE_TM_GMTOFF=1 HAVE_TIMEZONE_VAR=1
        HAVE_STRUCT_STAT_ST_BLOCKS=1 HAVE_STRUCT_STAT_ST_BLKSIZE=1 HAVE_BLKCNT_T=1
        NO_UNION_WAIT=1 HAVE_SIGNED_CHAR=1 HAVE_LANGINFO=1 HAVE_MKSTEMPS=1 HAVE_FTS=1
        TCL_UNLOAD_DLLS=1
        # tclUnixInit.c's library and package paths; openMSX does not run Tcl_Init, so unused.
        TCL_LIBRARY="" TCL_PACKAGE_PATH="")
endif()
set(TCL_DEFINITIONS
    BUILD_tcl STATIC_BUILD=1 PACKAGE_NAME="tcl" PACKAGE_TARNAME="tcl" PACKAGE_VERSION="8.6"
    PACKAGE_STRING="tcl 8.6" PACKAGE_BUGREPORT="" STDC_HEADERS=1 HAVE_SYS_TYPES_H=1
    HAVE_SYS_STAT_H=1 HAVE_STDLIB_H=1 HAVE_STRING_H=1 HAVE_MEMORY_H=1 HAVE_STRINGS_H=1
    HAVE_INTTYPES_H=1 HAVE_STDINT_H=1 HAVE_UNISTD_H=1 USE_THREAD_ALLOC=1 TCL_THREADS=1 HAVE_ZLIB=1
    HAVE_CAST_TO_UNION=1 HAVE_INTPTR_T=1 HAVE_UINTPTR_T=1 NDEBUG=1 TCL_CFG_OPTIMIZED=1 MP_PREC=4
    MP_FIXED_CUTOFFS MP_NO_STDINT
    # tclPkgConfig.c's install locations, as "tcl::pkgconfig" reports them; there is no install.
    CFG_INSTALL_LIBDIR="" CFG_INSTALL_BINDIR="" CFG_INSTALL_SCRDIR="" CFG_INSTALL_INCDIR=""
    CFG_INSTALL_DOCDIR="" CFG_RUNTIME_LIBDIR="" CFG_RUNTIME_BINDIR="" CFG_RUNTIME_SCRDIR=""
    CFG_RUNTIME_INCDIR="" CFG_RUNTIME_DOCDIR="" CFG_RUNTIME_DLLFILE=""
    ${TCL_PLATFORM_DEFINITIONS})
set(TCL_INCLUDES "${TCL_DIR}/generic" "${TCL_PLATFORM_DIR}" "${TCL_DIR}/libtommath")
add_library(kairo_tcl STATIC ${TCL_SOURCES})
target_include_directories(kairo_tcl PUBLIC "${TCL_DIR}/generic" "${TCL_PLATFORM_DIR}")
target_include_directories(kairo_tcl PRIVATE ${TCL_INCLUDES})
target_compile_definitions(kairo_tcl PUBLIC STATIC_BUILD=1)
target_compile_definitions(kairo_tcl PRIVATE ${TCL_DEFINITIONS})
if(WIN32)
    # Tcl's Windows build compiles tclMain.c twice; the wide-character copy (tclMainW) holds the
    # functions both share.
    add_library(kairo_tcl_mainw OBJECT "${TCL_DIR}/generic/tclMain.c")
    target_include_directories(kairo_tcl_mainw PRIVATE ${TCL_INCLUDES})
    target_compile_definitions(kairo_tcl_mainw PRIVATE ${TCL_DEFINITIONS} UNICODE _UNICODE)
    target_sources(kairo_tcl PRIVATE $<TARGET_OBJECTS:kairo_tcl_mainw>)
endif()
target_link_libraries(kairo_tcl PUBLIC kairo_zlib)
if(WIN32)
    target_link_libraries(kairo_tcl PUBLIC netapi32 kernel32 user32 advapi32 userenv ws2_32)
endif()

# SDL2, statically, with the subsystems openMSX's emulation code calls; openMSX starts none of
# them in KairoMSX (only its main() and its SDL video and sound drivers do). On Android, SDL is
# built without its bridge to its own Java classes (SDL_KAIRO_NO_JAVA; see the sdl-no-java change
# in tools/local_patches.py): the Kairo frontend owns the app's Java side and JNI_OnLoad.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
foreach(subsystem AUDIO VIDEO RENDER JOYSTICK HAPTIC HIDAPI POWER SENSOR LOCALE MISC)
    set(SDL_${subsystem} OFF CACHE BOOL "" FORCE)
endforeach()
foreach(subsystem ATOMIC EVENTS THREADS TIMERS FILE LOADSO CPUINFO FILESYSTEM)
    set(SDL_${subsystem} ON CACHE BOOL "" FORCE)
endforeach()
set(SDL_KAIRO_NO_JAVA ${ANDROID})
add_subdirectory("${THIRD_PARTY}/sdl2" sdl2 EXCLUDE_FROM_ALL)
if(ANDROID)
    target_compile_definitions(SDL2-static PRIVATE SDL_KAIRO_NO_JAVA)
endif()
