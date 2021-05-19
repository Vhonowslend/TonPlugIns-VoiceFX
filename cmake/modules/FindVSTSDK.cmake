# Distributed under MIT/X11 License. See https://en.wikipedia.org/wiki/MIT_License for more info.

#[=======================================================================[.rst:
FindVSTSDK
--------

Find the native VSTSDK headers and libraries.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``NVIDIA::AudioEffects``, if
VSTSDK has been found.

Result Variables
^^^^^^^^^^^^^^^^


#]=======================================================================]

include(FindPackageHandleStandardArgs)

# Variables
set(VSTSDK_DIR "${VSTSDK_DIR}" CACHE PATH "Path to Steinberg VST3SDK")

# Find VST SDK Directory
find_path(VSTSDK_VST3_DIR
	NAMES
		"public.sdk"
	HINTS
		$ENV{VSTSDK_DIR}
		${VSTSDK_DIR}
)
set(VSTSDK_INCLUDE_DIRS ${VSTSDK_VST3_DIR} ${VSTSDK_VST2_DIR})
if(WIN32)
	set(VSTSDK_VST3_EXPORTS_FILE ${VSTSDK_VST3_DIR}/public.sdk/source/main/winexport.def)
elseif(APPLE)
	set(VSTSDK_VST3_EXPORTS_FILE ${VSTSDK_VST3_DIR}/public.sdk/source/main/macexport.exp)
endif()

# Handle components
foreach(_component ${VSTSDK_FIND_COMPONENTS})
	if(_component STREQUAL "VST2WRAPPER")
		if(EXISTS "${VSTSDK_VST3_DIR}/public.sdk/source/vst/vst2wrapper/vst2wrapper.h")
			set(VSTSDK_VST2WRAPPER_FOUND ON)
		endif()
	elseif(_component STREQUAL "VSTGUI")
		if(EXISTS "${VSTSDK_VST3_DIR}/vstgui4/vstgui/vstgui.h")
			set(VSTSDK_VSTGUI_FOUND ON)
		endif()
	endif()
endforeach()

# Do things
find_package_handle_standard_args(VSTSDK
	FOUND_VAR VSTSDK_FOUND
	REQUIRED_VARS VSTSDK_INCLUDE_DIRS
	HANDLE_COMPONENTS
)

if(VSTSDK_FOUND AND NOT TARGET VSTSDK_VST3)
	add_library(VSTSDK_VST3 OBJECT
	# Base
		${VSTSDK_VST3_DIR}/base/source/baseiids.cpp
		${VSTSDK_VST3_DIR}/base/source/classfactoryhelpers.h
		${VSTSDK_VST3_DIR}/base/source/fbuffer.cpp
		${VSTSDK_VST3_DIR}/base/source/fbuffer.h
		${VSTSDK_VST3_DIR}/base/source/fcleanup.h
		${VSTSDK_VST3_DIR}/base/source/fcommandline.h
		${VSTSDK_VST3_DIR}/base/source/fdebug.cpp
		${VSTSDK_VST3_DIR}/base/source/fdebug.h
		${VSTSDK_VST3_DIR}/base/source/fdynlib.cpp
		${VSTSDK_VST3_DIR}/base/source/fdynlib.h
		${VSTSDK_VST3_DIR}/base/source/fobject.cpp
		${VSTSDK_VST3_DIR}/base/source/fobject.h
		${VSTSDK_VST3_DIR}/base/source/fstdmethods.h
		${VSTSDK_VST3_DIR}/base/source/fstreamer.cpp
		${VSTSDK_VST3_DIR}/base/source/fstreamer.h
		${VSTSDK_VST3_DIR}/base/source/fstring.cpp
		${VSTSDK_VST3_DIR}/base/source/fstring.h
		${VSTSDK_VST3_DIR}/base/source/hexbinary.h
		${VSTSDK_VST3_DIR}/base/source/timer.cpp
		${VSTSDK_VST3_DIR}/base/source/timer.h
		${VSTSDK_VST3_DIR}/base/source/updatehandler.cpp
		${VSTSDK_VST3_DIR}/base/source/updatehandler.h
		${VSTSDK_VST3_DIR}/base/thread/include/fcondition.h
		${VSTSDK_VST3_DIR}/base/thread/source/fcondition.cpp
		${VSTSDK_VST3_DIR}/base/thread/include/flock.h
		${VSTSDK_VST3_DIR}/base/thread/source/flock.cpp
	# PlugInterfaces
		${VSTSDK_VST3_DIR}/pluginterfaces/base/conststringtable.cpp
		${VSTSDK_VST3_DIR}/pluginterfaces/base/conststringtable.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/coreiids.cpp
		${VSTSDK_VST3_DIR}/pluginterfaces/base/doc.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/falignpop.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/falignpush.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/fplatform.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/fstrdefs.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ftypes.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/funknown.cpp
		${VSTSDK_VST3_DIR}/pluginterfaces/base/funknown.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/futils.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/fvariant.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/geoconstants.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ibstream.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/icloneable.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ierrorcontext.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ipersistent.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ipluginbase.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/istringresult.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/iupdatehandler.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/keycodes.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/pluginbasefwd.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/smartpointer.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/typesizecheck.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ucolorspec.h
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ustring.cpp
		${VSTSDK_VST3_DIR}/pluginterfaces/base/ustring.h
		${VSTSDK_VST3_DIR}/pluginterfaces/gui/iplugview.h
		${VSTSDK_VST3_DIR}/pluginterfaces/gui/iplugviewcontentscalesupport.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstattributes.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstaudioprocessor.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstautomationstate.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstchannelcontextinfo.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstcomponent.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstcontextmenu.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivsteditcontroller.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstevents.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivsthostapplication.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstinterappaudio.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstmessage.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstmidicontrollers.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstmidilearn.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstnoteexpression.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstparameterchanges.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstparameterfunctionname.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstphysicalui.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstpluginterfacesupport.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstplugview.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstprefetchablesupport.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstprocesscontext.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstrepresentation.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivsttestplugprovider.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/ivstunits.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/vstpresetkeys.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/vstpshpack4.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/vstspeaker.h
		${VSTSDK_VST3_DIR}/pluginterfaces/vst/vsttypes.h
	# SDK
#		${VSTSDK_VST3_DIR}/public.sdk/source/vst3stdsdk.cpp
	# SDK - Common
		${VSTSDK_VST3_DIR}/public.sdk/source/common/commoniids.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/common/pluginview.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/common/pluginview.h
		${VSTSDK_VST3_DIR}/public.sdk/source/common/threadchecker.h
	# Main
		${VSTSDK_VST3_DIR}/public.sdk/source/main/pluginfactory.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/main/pluginfactory.h
	# VSTSDK
		${VSTSDK_VST3_DIR}/public.sdk/source/common/memorystream.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/common/memorystream.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/utility/mpeprocessor.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/utility/mpeprocessor.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/utility/ringbuffer.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstaudioeffect.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstaudioeffect.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstaudioprocessoralgo.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstbus.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstbus.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstbypassprocessor.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstcomponent.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstcomponent.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstcomponentbase.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstcomponentbase.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vsteditcontroller.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vsteditcontroller.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vsteventshelper.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vsthelpers.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstinitiids.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstnoteexpressiontypes.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstnoteexpressiontypes.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstparameters.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstparameters.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstpresetfile.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstpresetfile.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstrepresentation.cpp
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstrepresentation.h
#		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
#		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.h
		${VSTSDK_VST3_DIR}/public.sdk/source/vst/vstspeakerarray.h
	)
	if(WIN32)
		target_sources(VSTSDK_VST3 PRIVATE
			${VSTSDK_VST3_DIR}/public.sdk/source/common/threadchecker_win32.cpp
			${VSTSDK_VST3_DIR}/public.sdk/source/main/dllmain.cpp
		)
	elseif(APPLE)
		target_sources(VSTSDK_VST3 PRIVATE
			${VSTSDK_VST3_DIR}/public.sdk/source/common/threadchecker_mac.mm
			${VSTSDK_VST3_DIR}/public.sdk/source/main/macmain.cpp
		)
	else()
		target_sources(VSTSDK_VST3 PRIVATE
			${VSTSDK_VST3_DIR}/public.sdk/source/common/threadchecker_linux.cpp
			${VSTSDK_VST3_DIR}/public.sdk/source/main/linuxmain.cpp
		)
	endif()
	target_include_directories(VSTSDK_VST3
		PRIVATE
			${VSTSDK_VST3_DIR}
		INTERFACE
			${VSTSDK_VST3_DIR}
	)
endif()
if(VSTSDK_VSTGUI_FOUND AND NOT TARGET VSTSDK_VSTGUI)
	message(FATAL_ERROR "VSTGUI support is currently not implemented")
#	include(${VSTSDK_VST3_DIR}/vstgui4/vstgui/CMakeLists.txt)
endif()

if(TARGET VSTSDK_VST3)
	get_target_property(tmp VSTSDK_VST3 SOURCES)
	source_group(TREE "${VSTSDK_VST3_DIR}" FILES ${tmp})
endif()
