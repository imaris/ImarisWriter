
# TARGET and CONFIG should be specified by the caller script

TEMPLATE = lib
QT -= gui core

include(../../../../bpQmakeCommonOptions.pro)

VERSION = $${BPIMARISWRITER_MAJORVERSION}.$${BPIMARISWRITER_MINORVERSION}.$${BPIMARISWRITER_PATCHVERSION}


INCLUDEPATH += \
        ../../../../libs 

INCLUDEPATH += \
        $$(BP_OUTPUTDIR)/thirdparty/boost/include \
		$$(BP_OUTPUTDIR)/thirdparty/FreeImage/include \
        $$(BP_OUTPUTDIR)/thirdparty/zlib/include \
        $$(BP_OUTPUTDIR)/thirdparty/lz4/include \
        $$(BP_OUTPUTDIR)/thirdparty/hdf5/include

DEPENDPATH += $${INCLUDEPATH}


DEFINES -= BPIMARISWRITER_DLL

# include qt header files as system header files (see bpQmakeCommonOptions.pro)
# in order to get rid of compiler warnings
macx {
    QMAKE_INCDIR_QT =
    QMAKE_CXXFLAGS += -isystem $$(BP_OUTPUTDIR)/thirdparty/boost/include
}

include(ImarisWriter.pri)
PROJECTDIRECTORY=libs/ImarisWriter/project/qt
include(../../../../bpQmakeCommonFileCheck.pro)
