#############################################################################
#
# ViSP, open source Visual Servoing Platform software.
# Copyright (C) 2005 - 2023 by Inria. All rights reserved.
#
# This software is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# See the file LICENSE.txt at the root directory of this source
# distribution for additional information about the GNU GPL.
#
# For using ViSP with software that can not be combined with the GNU
# GPL, please contact Inria about acquiring a ViSP Professional
# Edition License.
#
# See https://visp.inria.fr for more information.
#
# This software was developed at:
# Inria Rennes - Bretagne Atlantique
# Campus Universitaire de Beaulieu
# 35042 Rennes Cedex
# France
#
# If you have questions regarding the use of this file, please contact
# Inria at visp@inria.fr
#
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Description:
# ViSP configuration file.
#
#############################################################################

# Remove BUILD_INTERFACE from __include_dirs
# IN/OUT: __include_dirs
#
# If __include_dirs contains "$<BUILD_INTERFACE:/home/VTK/install/include/vtk-9.3>" as input,
# it will be filtered as output to /home/VTK/install/include/vtk-9.3
macro(vp_filter_build_interface __include_dirs)
  if(${__include_dirs})
    set(__include_dirs_filtered)
    foreach(inc_ ${${__include_dirs}})
      string(REGEX REPLACE "\\$<BUILD_INTERFACE:" "" inc_ ${inc_})
      string(REGEX REPLACE ">" "" inc_ ${inc_})
      list(APPEND __include_dirs_filtered ${inc_})
    endforeach()

    set(${__include_dirs} ${__include_dirs_filtered})
  endif()
endmacro()

# Find pcl libraries and dependencies
# IN: pcl_libraries
# OUT: pcl_deps_include_dirs
# OUT: pcl_deps_libraries
#
# PCL_LIBRARIES contains VTK 3rd party such as vtkalglib and not /usr/local/Cellar/vtk/6.3.0/lib/libvtkalglib-6.3.1.dylib
# full path as requested to use ViSP as 3rd party. This is the case for all VTK libraries that are PCL dependencies.
# The build of ViSP works with PCL_LIBRARIES since in that case thanks to vtkalglib properties, CMake
# is able to find the real name and location of the libraries.
# But when ViSP is used as a 3rd party where it should import PCL libraries, it doesn't work with
# PCL_LIBRARIES and especially with VTK_LIBRARIES.
# The solution here is to get the full location of VTK_LIBRARIES libraries thanks to the properties and link
# with these names.
# An other way could be to include PCLConfig.cmake, but in that case, visp-config and visp.pc
# will be not able to give the names of PCL libraries when used without CMake.
#
macro(vp_find_pcl pcl_libraries pcl_deps_include_dirs pcl_deps_libraries pcl_deps_compile_options)
  # Get compile options like "$<$<COMPILE_LANGUAGE:CXX>:-msse4.2;-mfpmath=sse;-march=native;-mavx2>"
  # end filter them out to keep "-msse4.2;-mfpmath=sse;-march=native;-mavx2"
  foreach(lib_ ${${pcl_libraries}})
    if(lib_ MATCHES "^pcl")
      get_target_property(imported_compile_options_ ${lib_} INTERFACE_COMPILE_OPTIONS)
      foreach(imported_compile_option_ ${imported_compile_options_})
        string(REGEX REPLACE "\\$<\\$<COMPILE_LANGUAGE:CXX>:" "" imported_compile_option_ ${imported_compile_option_})
        string(REGEX REPLACE ">" "" imported_compile_option_ ${imported_compile_option_})
        if(imported_compile_option_)
          list(APPEND ${pcl_deps_compile_options} ${imported_compile_option_})
        endif()
      endforeach()
    endif()
  endforeach()
  vp_list_unique(${pcl_deps_compile_options})

  # On macOS with PCL 1.14.3 or 1.15.0 PCLConfig.cmake propagates '-ffloat-store' in INTERFACE_COMPILE_OPTIONS
  # that lead to clang++: warning: optimization flag '-ffloat-store' is not supported [-Wignored-optimization-argument]
  # That's why here, we add the additional flag '-Wno-ignored-optimization-argument' to mute this warning
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    vp_check_flag_support(CXX "-Wno-ignored-optimization-argument" _varname "${VISP_EXTRA_CXX_FLAGS}")
    if(${_varname})
      list(APPEND ${pcl_deps_compile_options} "-Wno-ignored-optimization-argument")
    endif()
  endif()

  foreach(lib_ ${${pcl_libraries}})
    mark_as_advanced(${lib_}_LOCATION)
    if(TARGET ${lib_})
      # This is a PCL or VTK library
      list(APPEND PCL_VTK_LIBRARIES ${lib_})
      if(lib_ MATCHES "^FLANN")
        mark_as_advanced(flann_DIR)
      endif()
    else()
      # Other libraries sqlite3, boost..., optimized, debug
      if(EXISTS ${lib_} OR ${lib_} MATCHES "optimized" OR ${lib_} MATCHES "debug")
        list(APPEND ${pcl_deps_libraries} ${lib_})
      else()
        find_library(${lib_}_LIBRARY ${lib_} QUIET)
        mark_as_advanced(${lib_}_LIBRARY)
        if(${lib_}_LIBRARY)
          list(APPEND ${pcl_deps_libraries} ${${lib_}_LIBRARY})
        else()
          # Here on OSX with PCL 1.11.1, ${lib_} may contain
          # /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/lib/libz.tbd
          # /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/lib/libexpat.tbd
          # that doesn't exists. That's why next line was removed.
          #list(APPEND ${pcl_deps_libraries} ${lib_})
        endif()
      endif()
    endif()
  endforeach()
  find_package(VTK QUIET)
  if (VTK_FOUND AND NOT ANDROID)
    # Fix for Ubuntu 16.04 to add vtkFiltering as dependency. Note that vtkFiltering doesn't exists on OSX
    list(FIND VTK_LIBRARIES "vtkFiltering" vtkFiltering_exists_)
    if(NOT ${vtkFiltering_exists_} EQUAL -1)
      list(APPEND PCL_VTK_LIBRARIES "vtkFiltering") # seems required on Ubuntu 16.04
    endif()
    if(VTK_VERSION VERSION_EQUAL 6.2.0)
      # Work around to avoid build issue on ubuntu 16.04 with libvtk6-dev package
      # cannot find -lvtkproj4
      # See https://bugs.launchpad.net/ubuntu/+source/vtk6/+bug/1573234
      list(REMOVE_ITEM ${pcl_deps_libraries} "vtkproj4")
    endif()
    # Get pcl link libraries like opengl
    vp_list_unique(PCL_VTK_LIBRARIES)
    if(NOT VTK_ENABLE_KITS)
      foreach(lib_ ${PCL_VTK_LIBRARIES})
        get_target_property(imported_libs_ ${lib_} INTERFACE_LINK_LIBRARIES)
        if(imported_libs_)
          list(APPEND PCL_VTK_LIBS ${lib_})
          list(APPEND PCL_VTK_IMPORTED_LIBS ${imported_libs_})
        else()
          list(APPEND PCL_VTK_LIBS ${lib_})
        endif()

        get_target_property(imported_incs_ ${lib_} INTERFACE_INCLUDE_DIRECTORIES)
        if(imported_incs_)
          list(APPEND PCL_VTK_IMPORTED_INCS ${imported_incs_})
        endif()
      endforeach()
      vp_list_unique(PCL_VTK_IMPORTED_LIBS)
      vp_list_unique(PCL_VTK_IMPORTED_INCS)

      # Filter "$<BUILD_INTERFACE:/home/VTK/install/include/vtk-9.3>" into /home/VTK/install/include/vtk-9.3
      vp_filter_build_interface(PCL_VTK_IMPORTED_INCS)

      list(APPEND ${pcl_deps_include_dirs} ${PCL_VTK_IMPORTED_INCS})

      # Filter "\$<LINK_ONLY:vtkCommonMath>;\$<LINK_ONLY:opengl32>;\$<LINK_ONLY:glu32>" into "vtkCommonMath;opengl32;glu32"
      # Filter -lm into find_library(m) to get the full path of the library
      set(PCL_VTK_IMPORTED_LIBS_FILTERED)
      foreach(lib_ ${PCL_VTK_IMPORTED_LIBS})
        mark_as_advanced(${lib_}_LOCATION)
        string(REGEX REPLACE "\\$<LINK_ONLY:" "" lib_ ${lib_})
        string(REGEX REPLACE ">" "" lib_ ${lib_})

        if(lib_ MATCHES "^-l")
          string(REGEX REPLACE "-l" "" lib_ ${lib_})
          find_library(${lib_}_LOCATION ${lib_} QUIET)
          if(${lib_}_LOCATION)
            set(lib_ ${${lib_}_LOCATION})
          endif()
          mark_as_advanced(${lib}_LOCATION)
        elseif(TARGET ${lib_})
          find_library(${lib_}_LOCATION NAMES ${lib_} QUIET)
          if(${lib_}_LOCATION)
            set(lib_ ${${lib_}_LOCATION})
          endif()
        endif()

        list(APPEND PCL_VTK_IMPORTED_LIBS_FILTERED ${lib_})
      endforeach()
      set(PCL_VTK_IMPORTED_LIBS ${PCL_VTK_IMPORTED_LIBS_FILTERED})

      while(PCL_VTK_IMPORTED_LIBS)
        vp_list_pop_front(PCL_VTK_IMPORTED_LIBS elt)
        if(elt MATCHES "^vtk" OR elt MATCHES "^pcl") # to avoid processing -framework ApplicationServices -framework CoreServices
          get_target_property(imported_libs_ ${elt} INTERFACE_LINK_LIBRARIES)
          if(imported_libs_)
            list(APPEND PCL_VTK_IMPORTED_LIBS ${imported_libs_})
          else()
            list(APPEND PCL_VTK_LIBS ${elt})
          endif()
        else()
          list(APPEND PCL_VTK_LIBS ${elt})
        endif()
        vp_list_unique(PCL_VTK_IMPORTED_LIBS)
      endwhile()
      vp_list_unique(PCL_VTK_LIBS)
    else()
      foreach(lib_ ${PCL_VTK_LIBRARIES})
        get_target_property(imported_libs_ ${lib_} INTERFACE_LINK_LIBRARIES)
        if(imported_libs_)
          list(APPEND PCL_VTK_IMPORTED_LIBS ${imported_libs_})
          if(${lib_}_KIT)
            list(APPEND PCL_VTK_LIBS ${${lib_}_KIT})
          endif()
        else()
          list(APPEND PCL_VTK_LIBS ${lib_})
        endif()

        get_target_property(imported_incs_ ${lib_} INTERFACE_INCLUDE_DIRECTORIES)
        if(imported_incs_)
          list(APPEND PCL_VTK_IMPORTED_INCS ${imported_incs_})
        endif()
      endforeach()
      vp_list_unique(PCL_VTK_IMPORTED_LIBS)
      vp_list_unique(PCL_VTK_IMPORTED_INCS)

      # Filter "$<BUILD_INTERFACE:/home/VTK/install/include/vtk-9.3>" into /home/VTK/install/include/vtk-9.3
      vp_filter_build_interface(PCL_VTK_IMPORTED_INCS)

      list(APPEND ${pcl_deps_include_dirs} ${PCL_VTK_IMPORTED_INCS})

      while(PCL_VTK_IMPORTED_LIBS)
        vp_list_pop_front(PCL_VTK_IMPORTED_LIBS elt)
        if(elt MATCHES "^vtk" OR elt MATCHES "^pcl") # to avoid precessing -framework ApplicationServices -framework CoreServices
          get_target_property(imported_libs_ ${elt} INTERFACE_LINK_LIBRARIES)
          if(imported_libs_)
            list(APPEND PCL_VTK_IMPORTED_LIBS ${imported_libs_})
          else()
            list(APPEND PCL_VTK_LIBS ${elt})
          endif()
        else()
          list(APPEND PCL_VTK_LIBS ${elt})
        endif()
        vp_list_unique(PCL_VTK_IMPORTED_LIBS)
      endwhile()
      vp_list_unique(PCL_VTK_LIBS)
    endif()

    set(config_ "NONE" "RELEASE" "DEBUG" "RELEASEWITHDEBINFO" "RELWITHDEBINFO")
    foreach(lib_ ${PCL_VTK_LIBS})
      if(TARGET ${lib_})
        get_target_property(target_type_ ${lib_} TYPE)
        if(target_type_ STREQUAL "INTERFACE_LIBRARY")
          continue()
        endif()
        foreach(imp_config_ ${config_})
          get_target_property(lib_property_${imp_config_}_ ${lib_} IMPORTED_IMPLIB_${imp_config_})
          if(NOT EXISTS ${lib_property_${imp_config_}_})
            get_target_property(lib_property_${imp_config_}_ ${lib_} IMPORTED_LOCATION_${imp_config_})
          endif()
          # Under Unix, there is no specific suffix for PCL/VTK libraries.         # Under Windows, we add the "optimized", "debug" specific keywords
          if(WIN32 AND EXISTS "${lib_property_${imp_config_}_}" AND "${imp_config_}" MATCHES "RELEASE") # also valid for RELEASEWITHDEBINFO
            list(APPEND ${pcl_deps_libraries} optimized "${lib_property_${imp_config_}_}")
          elseif(WIN32 AND EXISTS "${lib_property_${imp_config_}_}" AND "${imp_config_}" MATCHES "DEBUG")
            list(APPEND ${pcl_deps_libraries} debug     "${lib_property_${imp_config_}_}")
          elseif(EXISTS "${lib_property_${imp_config_}_}")
            list(APPEND ${pcl_deps_libraries} "${lib_property_${imp_config_}_}")
          endif()
        endforeach()
      elseif(EXISTS ${lib_})
        list(APPEND ${pcl_deps_libraries} "${lib_}")
      else()
        # ${lib_} contains standalone lib like "m" or OSX frameworks like "-framework ApplicationServices -framework CoreServices"
        # or non existing libs like /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/lib/libz.tbd
        find_library(${lib_}_LOCATION ${lib_} QUIET)
        if(${lib_}_LOCATION)
          list(APPEND ${pcl_deps_libraries} ${${lib_}_LOCATION})
        elseif(lib_ MATCHES "^-framework")
          # Here ${lib_} may contain "-framework ApplicationServices -framework CoreServices"
          # "-framework Cocoa"
          list(APPEND ${pcl_deps_libraries} ${lib_})
        else()
          # Here ${lib_} may contain "/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/lib/libz.tbd"
          # that doesn't exists. We don't add it.
          #list(APPEND ${pcl_deps_libraries} ${lib_})
        endif()
      endif()
    endforeach()

    find_path(VTK_NLOHMANN_JSON_INCLUDE_DIR vtknlohmannjson/include/vtknlohmann/json.hpp
      PATHS
        ${PCL_VTK_IMPORTED_INCS}
    )
    mark_as_advanced(VTK_NLOHMANN_JSON_INCLUDE_DIR)
    if(VTK_NLOHMANN_JSON_INCLUDE_DIR)
      vp_parse_header("${VTK_NLOHMANN_JSON_INCLUDE_DIR}/vtknlohmannjson/include/vtknlohmann/json.hpp" NLOHMANN_JSON_VERSION_LINES NLOHMANN_JSON_VERSION_MAJOR NLOHMANN_JSON_VERSION_MINOR NLOHMANN_JSON_VERSION_PATCH)
      set(VTK_NLOHMANN_JSON_VERSION "${NLOHMANN_JSON_VERSION_MAJOR}.${NLOHMANN_JSON_VERSION_MINOR}.${NLOHMANN_JSON_VERSION_PATCH}")
      list(APPEND ${pcl_deps_include_dirs} "${VTK_NLOHMANN_JSON_INCLUDE_DIR}/vtknlohmannjson/include")
      set(VISP_HAVE_NLOHMANN_JSON_FROM_VTK TRUE)
    else()
      set(VISP_HAVE_NLOHMANN_JSON_FROM_VTK FALSE)
      set(VTK_NLOHMANN_JSON_VERSION "n/a")
    endif()

    # On win10 + msvc 15 2017 with pcl 1.9.1 opengl32.lib needed by vtkRenderingOpenGL-8.1-gd.lib is not found
    # Here we explicitly add opengl
    if(OPENGL_LIBRARIES)
      list(APPEND ${pcl_deps_libraries} ${OPENGL_LIBRARIES})
    endif()
  endif()

  mark_as_advanced(lib_LOCATION)
  mark_as_advanced(PCL_DIR)
  mark_as_advanced(PCL_APPS_LIBRARY)
  mark_as_advanced(PCL_APPS_LIBRARY_DEBUG)
  mark_as_advanced(pkgcfg_lib_FLANN_flann)
  mark_as_advanced(pkgcfg_lib_FLANN_flann_cpp)
  mark_as_advanced(pkgcfg_lib_FLANN_lz4)
  mark_as_advanced(pkgcfg_lib_GLEW_GLEW)
  mark_as_advanced(pkgcfg_lib_PC_FLANN_flann pkgcfg_lib_PC_FLANN_flann_cpp)
  mark_as_advanced(pkgcfg_lib_PC_OPENNI2_DummyDev)
  mark_as_advanced(pkgcfg_lib_PC_OPENNI2_DummyDevice)
  mark_as_advanced(pkgcfg_lib_PC_OPENNI2_OniFile)
  mark_as_advanced(pkgcfg_lib_PC_OPENNI2_OpenNI2 pkgcfg_lib_PC_OPENNI2_OpenNI2 pkgcfg_lib_PC_OPENNI_OpenNI)
  mark_as_advanced(pkgcfg_lib_PC_OPENNI2_PS1080)
  mark_as_advanced(pkgcfg_lib_PC_USB_10_usb-1.0)
  foreach(component ${PCL_TO_FIND_COMPONENTS})
    string(TOUPPER "${component}" COMPONENT)
    if(PCL_${COMPONENT}_INCLUDE_DIR)
      mark_as_advanced(PCL_${COMPONENT}_INCLUDE_DIR)
    endif()
  endforeach()

  mark_as_advanced(DAVIDSDK_INCLUDE_DIR DAVIDSDK_LIBRARY RSSDK_DIR DSSDK_DIR VTK_DIR)

  mark_as_advanced(EIGEN_INCLUDE_DIRS)

  mark_as_advanced(ENSENSO_INCLUDE_DIR ENSENSO_LIBRARY)

  mark_as_advanced(freetype_DIR)

  mark_as_advanced(flann_DIR)
  mark_as_advanced(FLANN_INCLUDE_DIR)
  mark_as_advanced(FLANN_INCLUDE_DIRS)
  mark_as_advanced(FLANN_LIBRARY)
  mark_as_advanced(FLANN_LIBRARY_DEBUG)
  mark_as_advanced(FLANN_LIBRARY_DEBUG_SHARED) # Requested on macOS with pcl 1.12.1
  mark_as_advanced(FLANN_LIBRARY_DEBUG_STATIC) # Requested on macOS with pcl 1.12.1
  mark_as_advanced(FLANN_LIBRARY_SHARED)       # Requested on macOS with pcl 1.12.1
  mark_as_advanced(FLANN_LIBRARY_STATIC)       # Requested on macOS with pcl 1.12.1

  mark_as_advanced(GLEW_INCLUDE_DIR GLEW_GLEW_LIBRARY GLEW_cocoa_LIBRARY)

  mark_as_advanced(HDF5_C_LIBRARY_dl)          # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_hdf5)        # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_hdf5_hl)     # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_m)           # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_sz)          # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_z)           # Requested on macOS with pcl 1.12.1
  mark_as_advanced(HDF5_C_LIBRARY_crypto)
  mark_as_advanced(HDF5_C_LIBRARY_curl)
  mark_as_advanced(HDF5_C_LIBRARY_pthread)
  mark_as_advanced(HDF5_DIR)

  mark_as_advanced(ICU_INCLUDE_DIR)            # Requested on macOS with pcl 1.12.1

  mark_as_advanced(LZMA_INCLUDE_DIR)           # Requested on macOS with pcl 1.12.1
  mark_as_advanced(LZMA_LIBRARY)               # Requested on macOS with pcl 1.12.1

  mark_as_advanced(QHULL_INCLUDE_DIRS)
  mark_as_advanced(QHULL_LIBRARY)
  mark_as_advanced(QHULL_LIBRARY_DEBUG)
  mark_as_advanced(QHULL_LIBRARY_STATIC)       # Requested for pcl 1.13.1 on windows
  mark_as_advanced(QHULL_LIBRARY_DEBUG_STATIC) # Requested for pcl 1.13.1 on windows
  mark_as_advanced(QHULL_LIBRARY_SHARED)       # Requested for pcl 1.13.1 on windows
  mark_as_advanced(Qhull_DIR)                  # Requested on macOS with pcl 1.12.1

  mark_as_advanced(Qt5Core_DIR Qt5Gui_DIR Qt5Network_DIR Qt5WebKit_DIR Qt5Widgets_DIR Qt5Sql_DIR)
  mark_as_advanced(Qt5OpenGL_DIR)              # Requested on macOS with pcl 1.12.1
  mark_as_advanced(Qt5QmlModels_DIR)           # Requested on macOS with pcl 1.12.1
  mark_as_advanced(Qt5Qml_DIR)                 # Requested on macOS with pcl 1.12.1
  mark_as_advanced(Qt5Quick_DIR)               # Requested on macOS with pcl 1.12.1
  mark_as_advanced(Qt5_DIR)                    # Requested on macOS with pcl 1.12.1

  # Requested on macOS with pcl 1.13.1
  mark_as_advanced(Qt6CoreTools_DIR Qt6Core_DIR Qt6BusTools_DIR Qt6GuiTools_DIR Qt6Gui_DIR Qt6OpenGLWidgets_DIR)
  mark_as_advanced(Qt6OpenGL_DIR Qt6WidgetsTools_DIR Qt6Widgets_DIR)
  mark_as_advanced(Qt6DBusTools_DIR Qt6DBus_DIR Qt6Network_DIR Qt6QmlCompilerPlusPrivate_DIR)
  mark_as_advanced(Qt6QmlIntegration_DIR Qt6QmlModels_DIR Qt6QmlTools_DIR Qt6Qml_DIR Qt6Quick_DIR Qt6Sql_DIR)
  mark_as_advanced(Qt6QmlBuiltins_DIR Qt6QuickTools_DIR)
  mark_as_advanced(Qt6_DIR)
  mark_as_advanced(QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH)
  mark_as_advanced(QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
  mark_as_advanced(MACDEPLOYQT_EXECUTABLE)
  mark_as_advanced(WrapOpenGL_AGL)

  mark_as_advanced(OPENNI2_INCLUDE_DIR)
  mark_as_advanced(OPENNI2_INCLUDE_DIRS)
  mark_as_advanced(OPENNI2_LIBRARY)

  mark_as_advanced(OPENNI_INCLUDE_DIR)
  mark_as_advanced(OPENNI_INCLUDE_DIRS)
  mark_as_advanced(OPENNI_LIBRARY)

  mark_as_advanced(PCAP_INCLUDE_DIR)                        # Requested on macOS with pcl 1.13.1
  mark_as_advanced(PCAP_LIBRARY)                            # Requested on macOS with pcl 1.13.1

  mark_as_advanced(USB_10_INCLUDE_DIR)
  mark_as_advanced(USB_10_LIBRARY)

  mark_as_advanced(Boost_INCLUDE_DIR)                       # Requested on macOS with pcl 1.12.1
  set(configuration "RELEASE;DEBUG")                        # Requested for pcl 1.13.1 on windows
  foreach(config ${configuration})
    mark_as_advanced(Boost_THREAD_LIBRARY_${config})        # Requested on Ubuntu 20.04
    mark_as_advanced(Boost_DATE_TIME_LIBRARY_${config})     # Requested on macOS with pcl 1.12.1
    mark_as_advanced(Boost_FILESYSTEM_LIBRARY_${config})    # Requested on macOS with pcl 1.12.1
    mark_as_advanced(Boost_IOSTREAMS_LIBRARY_${config})     # Requested on macOS with pcl 1.12.1
    mark_as_advanced(Boost_SERIALIZATION_LIBRARY_${config}) # Requested on macOS with pcl 1.12.1
    mark_as_advanced(Boost_SYSTEM_LIBRARY_${config})        # Requested on macOS with pcl 1.12.1
  endforeach()

  mark_as_advanced(libusb_INCLUDE_DIR)                  # Requested on macOS with pcl 1.12.1
  mark_as_advanced(netCDF_DIR)                          # Requested on macOS with pcl 1.12.1
  mark_as_advanced(pugixml_DIR)                         # Requested on macOS with pcl 1.12.1

  mark_as_advanced(OPENGL_GLES2_INCLUDE_DIR)
  mark_as_advanced(OPENGL_GLES3_INCLUDE_DIR)

  mark_as_advanced(VTK_MPI_NUMPROCS)
  mark_as_advanced(VTK_DIR)                             # Requested on macOS with pcl 1.12.1

  mark_as_advanced(TBB_DIR)
  mark_as_advanced(Tiff_DIR)
  mark_as_advanced(tiff_DIR)
  mark_as_advanced(CLI11_DIR)

  mark_as_advanced(synchronization_LOCATION)            # Requested for pcl 1.13.1 on windows
  mark_as_advanced($<$<CONFIG:debug:bcrypt_LOCATION)    # Requested for pcl 1.13.1 on windows
  mark_as_advanced($<$<CONFIG:release:bcrypt_LOCATION)            # Requested for pcl 1.13.1 on windows
endmacro()

# Find pcl modules
# IN: pcl_components
# OUT: none
#
macro(vp_detect_required_pcl_components pcl_components)
  foreach(component_ ${${pcl_components}})
    string(TOUPPER "${component_}" COMPONENT)
    if(PCL_${COMPONENT}_FOUND)
      set(VISP_HAVE_PCL_${COMPONENT} TRUE)
    endif()
  endforeach()
endmacro()
