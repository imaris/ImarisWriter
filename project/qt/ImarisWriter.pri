# -----------------------------------------------------------
SUBDIR = ../../interface
# -----------------------------------------------------------
HEADERS += \
$${SUBDIR}/bpConverterTypes.h \
$${SUBDIR}/bpImageConverterInterface.h \
$${SUBDIR}/bpImageConverter.h \
$${SUBDIR}/ImarisWriterDllAPI.h \


# -----------------------------------------------------------
SUBDIR = ../../interfaceC
# -----------------------------------------------------------
HEADERS += \
$${SUBDIR}/bpConverterTypesC.h \
$${SUBDIR}/bpImageConverterInterfaceC.h \


# -----------------------------------------------------------
SUBDIR = ../../c
# -----------------------------------------------------------
SOURCES += \
$${SUBDIR}/bpImageConverterC.cxx \


# -----------------------------------------------------------
SUBDIR = ../../writer
# -----------------------------------------------------------
HEADERS += \
$${SUBDIR}/bpCircularBuffer.h \
$${SUBDIR}/bpCompressionAlgorithm.h \
$${SUBDIR}/bpCompressionAlgorithmFactory.h \
$${SUBDIR}/bpDeriche.h \
$${SUBDIR}/bpGzip.h \
$${SUBDIR}/bpLZ4.h \
$${SUBDIR}/bpH5LZ4.h \
$${SUBDIR}/bpShuffle.h \
$${SUBDIR}/bpHistogram.h \
$${SUBDIR}/bpImageConverterImpl.h \
$${SUBDIR}/bpImsImage3D.h \
$${SUBDIR}/bpImsImage5D.h \
$${SUBDIR}/bpImsImageBlock.h \
$${SUBDIR}/bpImsLayout.h \
$${SUBDIR}/bpImsUtils.h \
$${SUBDIR}/bpImsLayout3D.h \
$${SUBDIR}/bpMemoryBlock.h \
$${SUBDIR}/bpMemoryManager.h \
$${SUBDIR}/bpMultiresolutionImsImage.h \
$${SUBDIR}/bpOptimalBlockLayout.h \
$${SUBDIR}/bpThreadPool.h \
$${SUBDIR}/bpThumbnail.h \
$${SUBDIR}/bpThumbnailBuilder.h \
$${SUBDIR}/bpWriter.h \
$${SUBDIR}/bpWriterCompressor.h \
$${SUBDIR}/bpWriterFactory.h \
$${SUBDIR}/bpWriterFactoryCompressor.h \
$${SUBDIR}/bpWriterFactoryHDF5.h \
$${SUBDIR}/bpWriterHDF5.h \
$${SUBDIR}/bpWriterThreads.h \


SOURCES += \
$${SUBDIR}/bpDeriche.cxx \
$${SUBDIR}/bpCompressionAlgorithmFactory.cxx \
$${SUBDIR}/bpGzip.cxx \
$${SUBDIR}/bpLZ4.cxx \
$${SUBDIR}/bpH5LZ4.cxx \
$${SUBDIR}/bpShuffle.cxx \
$${SUBDIR}/bpHistogram.cxx \
$${SUBDIR}/bpImageConverter.cxx \
$${SUBDIR}/bpImageConverterImpl.cxx \
$${SUBDIR}/bpImsImage3D.cxx \
$${SUBDIR}/bpImsImage5D.cxx \
$${SUBDIR}/bpImsImageBlock.cxx \
$${SUBDIR}/bpImsLayout.cxx \
$${SUBDIR}/bpImsLayout3D.cxx \
$${SUBDIR}/bpImsUtils.cxx \
$${SUBDIR}/bpMemoryManager.cxx \
$${SUBDIR}/bpMultiresolutionImsImage.cxx \
$${SUBDIR}/bpOptimalBlockLayout.cxx \
$${SUBDIR}/bpThreadPool.cxx \
$${SUBDIR}/bpThumbnailBuilder.cxx \
$${SUBDIR}/bpWriterCompressor.cxx \
$${SUBDIR}/bpWriterFactoryCompressor.cxx \
$${SUBDIR}/bpWriterFactoryHDF5.cxx \
$${SUBDIR}/bpWriterHDF5.cxx \
$${SUBDIR}/bpWriterThreads.cxx \

