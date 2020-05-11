########################## General Configuration
### Template is an dynamic library named ImarisWriter
TARGET = bpImarisWriter

include(ImarisWriter_lib.pro)

### config
CONFIG += dll

win32 {
    TARGET_VERSION_EXT = $${BPIMARISWRITER_MAJORVERSION}$${BPIMARISWRITER_MINORVERSION}
}

### qmake post link
macx {
    # set the install_name useful for developing
    QMAKE_POST_LINK += install_name_tool -id \
            @rpath/lib$${TARGET}.$${BPIMARISWRITER_MAJORVERSION}.$${BPIMARISWRITER_MINORVERSION}.$${DLLSUFFIX} \
            $${DESTDIR}/lib$${TARGET}.$${BPIMARISWRITER_MAJORVERSION}.$${BPIMARISWRITER_MINORVERSION}.$${DLLSUFFIX}
}

### libraries
LIBS += -L$$(BP_OUTPUTDIR)/$${CONFIGURATION}
LIBS += \
        $${LIBZLIB_LIB} \
        $${LIBLZ4_LIB} \
        $${LIBHDF_LIB}


# Note: on unix it is important to have the right order of libs to be linked to the exe


macx {
    #QMAKE_LFLAGS += -bind_at_load
    LIBS += \
            -framework Accelerate \
            -framework CoreServices \
            -framework SystemConfiguration \
            -framework Carbon}
