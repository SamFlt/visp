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

vp_add_module(imgproc visp_core WRAP java)

# You can add cmake specific material like Find<Package>.cmake files in a specific
# folder named for example "cmake" and uncomment the following line to use it.
# vp_add_cmake_module_path(cmake)

# You can add specific headers that are dynamically configured in
# the include folder of the module. Create for example the file
# cmake/templates/vpConfigImproc.h.in and uncomment the following line.
# vp_add_config_file(cmake/templates/vpConfigImproc.h.in)

vp_glob_module_sources()
vp_module_include_directories()
vp_create_module()

vp_add_tests(DEPENDS_ON visp_imgproc visp_io)
