/****************************************************************************
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2017 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See http://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * Example of tracking with vpGenericTracker on Castel.
 *
 *****************************************************************************/

/*!
  \example mbtGenericTrackingDepthOnly.cpp

  \brief Example of tracking with vpGenericTracker on Castel.
*/

#include <cstdlib>
#include <iostream>
#include <visp3/core/vpConfig.h>

#if defined(VISP_HAVE_DISPLAY)

#include <visp3/core/vpDebug.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/core/vpMath.h>
#include <visp3/gui/vpDisplayD3D.h>
#include <visp3/gui/vpDisplayGDI.h>
#include <visp3/gui/vpDisplayGTK.h>
#include <visp3/gui/vpDisplayOpenCV.h>
#include <visp3/gui/vpDisplayX.h>
#include <visp3/io/vpImageIo.h>
#include <visp3/io/vpParseArgv.h>
#include <visp3/io/vpVideoReader.h>
#include <visp3/mbt/vpMbGenericTracker.h>

#define GETOPTARGS "X:M:i:n:dchfolwvpT:e:u:"

#define USE_XML 1
#define USE_SMALL_DATASET 1 // small depth dataset in ViSP-images

// Detect endianness of the host machine
// Reference: http://www.boost.org/doc/libs/1_36_0/boost/detail/endian.hpp
#if defined(__GLIBC__)
#include <endian.h>
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define VISP_LITTLE_ENDIAN
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define VISP_BIG_ENDIAN
#elif (__BYTE_ORDER == __PDP_ENDIAN)
// Currently not supported when reading / writing binary file
#define VISP_PDP_ENDIAN
#else
#error Unknown machine endianness detected.
#endif
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN) || defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#define VISP_BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN) || defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#define VISP_LITTLE_ENDIAN
#elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) ||         \
    defined(__hpux) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)

#define VISP_BIG_ENDIAN
#elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) ||           \
    defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) ||            \
    defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)

#define VISP_LITTLE_ENDIAN
#else
#error Cannot detect host machine endianness.
#endif

namespace
{
#ifdef VISP_BIG_ENDIAN
// Swap 16 bits by shifting to the right the first byte and by shifting to the
// left the second byte
uint16_t swap16bits(const uint16_t val) { return (((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00)); }

// Swap 32 bits by shifting to the right the first 2 bytes and by shifting to
// the left the last 2 bytes
uint32_t swap32bits(const uint32_t val)
{
  return (((val >> 24) & 0x000000FF) | ((val >> 8) & 0x0000FF00) | ((val << 8) & 0x00FF0000) |
          ((val << 24) & 0xFF000000));
}
#endif

// Read an int stored in little endian
void readBinaryUIntLE(std::ifstream &file, unsigned int &uint_value)
{
  // Read
  file.read((char *)(&uint_value), sizeof(uint_value));

#ifdef VISP_BIG_ENDIAN
  // Swap bytes order from little endian to big endian
  if (sizeof(uint_value) == 4) {
    uint_value = (unsigned int)swap32bits((uint32_t)uint_value);
  } else {
    uint_value = swap16bits((uint16_t)uint_value);
  }
#endif
}

// Read an int stored in little endian
void readBinaryUIntLE(std::ifstream &file, uint16_t &uint_value)
{
  // Read
  file.read((char *)(&uint_value), sizeof(uint_value));

#ifdef VISP_BIG_ENDIAN
  // Swap bytes order from little endian to big endian
  uint_value = swap16bits((uint16_t)uint_value);
#endif
}

void usage(const char *name, const char *badparam)
{
  fprintf(stdout, "\n\
  Example of tracking with vpGenericTracker.\n\
  \n\
  SYNOPSIS\n\
    %s [-i <test image path>] [-X <config file depth>]\n\
    [-M <model name depth>] [-n <initialisation file base name>]\n\
    [-f] [-c] [-d] [-h] [-o] [-w] [-l] [-v] [-p]\n\
    [-T <tracker type>] [-e <last frame index>]\n\
    [-u <disable face>]\n", name);

  fprintf(stdout, "\n\
  OPTIONS:                                               \n\
    -i <input image path>                                \n\
       Set image input path.\n\
       These images come from ViSP-images-x.y.z.tar.gz available \n\
       on the ViSP website.\n\
       Setting the VISP_INPUT_IMAGE_PATH environment\n\
       variable produces the same behavior than using\n\
       this option.\n\
  \n\
    -X <config file>                                     \n\
       Set the config file (the xml file) to use for the depth sensor.\n\
       The config file is used to specify the parameters of the tracker.\n\
  \n\
    -M <model name>                                 \n\
       Specify the name of the file of the model for the depth sensor.\n\
       The model can either be a vrml model (.wrl) or a .cao file.\n\
  \n\
    -n <initialisation file base name>                                            \n\
       Base name of the initialisation file. The file will be 'base_name'.init .\n\
       This base name is also used for the optional picture specifying where to \n\
       click (a .ppm picture).\n\
  \n\
    -f \n\
       Turn off the display of the visual features. \n\
  \n\
    -d \n\
       Turn off the display.\n\
  \n\
    -c\n\
       Disable the mouse click. Useful to automate the \n\
       execution of this program without human intervention.\n\
  \n\
    -o\n\
       Use Ogre3D for visibility tests\n\
  \n\
    -w\n\
       When Ogre3D is enable [-o] show Ogre3D configuration dialog that allows to set the renderer.\n\
  \n\
    -l\n\
       Use the scanline for visibility tests.\n\
  \n\
    -v\n\
       Compute covariance matrix.\n\
  \n\
    -p\n\
       Compute gradient projection error.\n\
  \n\
    -T <tracker type>\n\
       Set tracker type (<4 (Depth normal)>, <8 (Depth dense)>, <12 (both)>) for depth sensor.\n\
  \n\
    -e <last frame index>\n\
       Specify the index of the last frame. Once reached, the tracking is stopped.\n\
  \n\
    -u <disable>\n\
       Disable castle element (1=floor, 2=front_door, 4=slope, 8=tower_front, 16=tower_left, 32=tower_right, 64=tower_back).\n\
  \n\
    -h \n\
       Print the help.\n\n");

  if (badparam)
    fprintf(stdout, "\nERROR: Bad parameter [%s]\n", badparam);
}

bool getOptions(int argc, const char **argv, std::string &ipath, std::string &configFile_depth,
                std::string &modelFile_depth, std::string &initFile, bool &displayFeatures, bool &click_allowed,
                bool &display, bool &useOgre, bool &showOgreConfigDialog, bool &useScanline, bool &computeCovariance,
                bool &projectionError, int &tracker_type_depth, int &lastFrame, int &disable_castle_faces)
{
  const char *optarg_;
  int c;
  while ((c = vpParseArgv::parse(argc, argv, GETOPTARGS, &optarg_)) > 1) {

    switch (c) {
    case 'i':
      ipath = optarg_;
      break;
    case 'X':
      configFile_depth = optarg_;
      break;
    case 'M':
      modelFile_depth = optarg_;
      break;
    case 'n':
      initFile = optarg_;
      break;
    case 'f':
      displayFeatures = false;
      break;
    case 'c':
      click_allowed = false;
      break;
    case 'd':
      display = false;
      break;
    case 'o':
      useOgre = true;
      break;
    case 'l':
      useScanline = true;
      break;
    case 'w':
      showOgreConfigDialog = true;
      break;
    case 'v':
      computeCovariance = true;
      break;
    case 'p':
      projectionError = true;
      break;
    case 'T':
      tracker_type_depth = atoi(optarg_);
      break;
    case 'e':
      lastFrame = atoi(optarg_);
      break;
    case 'u':
      disable_castle_faces = atoi(optarg_);
      break;

    case 'h':
      usage(argv[0], NULL);
      return false;
      break;
    default:
      usage(argv[0], optarg_);
      return false;
      break;
    }
  }

  if ((c == 1) || (c == -1)) {
    // standalone param or error
    usage(argv[0], NULL);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << "  Bad argument " << optarg_ << std::endl << std::endl;
    return false;
  }

  return true;
}

struct rs_intrinsics {
  float ppx;       /**< Horizontal coordinate of the principal point of the image,
                      as a pixel offset from the left edge */
  float ppy;       /**< Vertical coordinate of the principal point of the image, as
                      a pixel offset from the top edge */
  float fx;        /**< Focal length of the image plane, as a multiple of pixel width
                    */
  float fy;        /**< Focal length of the image plane, as a multiple of pixel
                      height */
  float coeffs[5]; /**< Distortion coefficients */
};

void rs_deproject_pixel_to_point(float point[3], const rs_intrinsics &intrin, const float pixel[2], float depth)
{
  float x = (pixel[0] - intrin.ppx) / intrin.fx;
  float y = (pixel[1] - intrin.ppy) / intrin.fy;

  float r2 = x * x + y * y;
  float f = 1 + intrin.coeffs[0] * r2 + intrin.coeffs[1] * r2 * r2 + intrin.coeffs[4] * r2 * r2 * r2;
  float ux = x * f + 2 * intrin.coeffs[2] * x * y + intrin.coeffs[3] * (r2 + 2 * x * x);
  float uy = y * f + 2 * intrin.coeffs[3] * x * y + intrin.coeffs[2] * (r2 + 2 * y * y);

  x = ux;
  y = uy;

  point[0] = depth * x;
  point[1] = depth * y;
  point[2] = depth;
}

bool read_data(const unsigned int cpt, const std::string &input_directory, vpImage<uint16_t> &I_depth_raw,
               std::vector<vpColVector> &pointcloud, unsigned int &pointcloud_width, unsigned int &pointcloud_height)
{
  char buffer[256];

  // Read raw depth
  std::stringstream ss;
  ss << input_directory << "/depth_image_%04d.bin";
  sprintf(buffer, ss.str().c_str(), cpt);
  std::string filename_depth = buffer;

  std::ifstream file_depth(filename_depth.c_str(), std::ios::in | std::ios::binary);
  if (!file_depth.is_open()) {
    return false;
  }

  unsigned int height = 0, width = 0;
  readBinaryUIntLE(file_depth, height);
  readBinaryUIntLE(file_depth, width);

  I_depth_raw.resize(height, width);

  uint16_t depth_value = 0;
  for (unsigned int i = 0; i < height; i++) {
    for (unsigned int j = 0; j < width; j++) {
      readBinaryUIntLE(file_depth, depth_value);
      I_depth_raw[i][j] = depth_value;
    }
  }

  // Transform pointcloud
  pointcloud_width = width;
  pointcloud_height = height;
  pointcloud.resize((size_t)width * height);

  // Only for Creative SR300
  const float depth_scale = 0.000124986647f;
  rs_intrinsics depth_intrinsic;
  depth_intrinsic.ppx = 311.484558f;
  depth_intrinsic.ppy = 246.283234f;
  depth_intrinsic.fx = 476.053619f;
  depth_intrinsic.fy = 476.053497f;
  depth_intrinsic.coeffs[0] = 0.165056542f;
  depth_intrinsic.coeffs[1] = -0.0508309528f;
  depth_intrinsic.coeffs[2] = 0.00435937941f;
  depth_intrinsic.coeffs[3] = 0.00541406544f;
  depth_intrinsic.coeffs[4] = 0.250085592f;

  for (unsigned int i = 0; i < height; i++) {
    for (unsigned int j = 0; j < width; j++) {
      float scaled_depth = I_depth_raw[i][j] * depth_scale;
      float point[3];
      float pixel[2] = {(float)j, (float)i};
      rs_deproject_pixel_to_point(point, depth_intrinsic, pixel, scaled_depth);

      vpColVector data_3D(3);
      data_3D[0] = point[0];
      data_3D[1] = point[1];
      data_3D[2] = point[2];

      pointcloud[(size_t)(i * width + j)] = data_3D;
    }
  }

  return true;
}

void loadConfiguration(vpMbTracker *const tracker, const std::string &
#if defined(VISP_HAVE_XML2) && USE_XML
                       configFile_depth
#endif
)
{
#if defined(VISP_HAVE_XML2) && USE_XML
  // From the xml file
  dynamic_cast<vpMbGenericTracker *>(tracker)->loadConfigFile(configFile_depth);
#else
  // Depth
  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthNormalFeatureEstimationMethod(
      vpMbtFaceDepthNormal::ROBUST_FEATURE_ESTIMATION);
  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthNormalPclPlaneEstimationMethod(2);
  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthNormalPclPlaneEstimationRansacMaxIter(200);
  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthNormalPclPlaneEstimationRansacThreshold(0.001);
  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthNormalSamplingStep(2, 2);

  dynamic_cast<vpMbGenericTracker *>(tracker)->setDepthDenseSamplingStep(4, 4);

  vpCameraParameters cam;
  cam.initPersProjWithoutDistortion(476.0536193848, 476.0534973145, 311.4845581055, 246.2832336426);

  dynamic_cast<vpMbGenericTracker *>(tracker)->setCameraParameters(cam);

  tracker->setAngleAppear(vpMath::rad(70));
  tracker->setAngleDisappear(vpMath::rad(80));

  // Specify the clipping to
  tracker->setNearClippingDistance(0.01);
  tracker->setFarClippingDistance(2.0);
  tracker->setClipping(tracker->getClipping() | vpMbtPolygon::FOV_CLIPPING);
//   tracker->setClipping(tracker->getClipping() | vpMbtPolygon::LEFT_CLIPPING
//   | vpMbtPolygon::RIGHT_CLIPPING | vpMbtPolygon::UP_CLIPPING |
//   vpMbtPolygon::DOWN_CLIPPING); // Equivalent to FOV_CLIPPING
#endif
}

std::vector<std::string> getCastleElementNames(const int element)
{
  std::vector<std::string> element_names;

  if (element & 0x1)
    element_names.push_back("floor");
  if (element & 0x2)
    element_names.push_back("front_door");
  if (element & 0x4)
    element_names.push_back("slope");
  if (element & 0x8)
    element_names.push_back("tower_front");
  if (element & 0x10)
    element_names.push_back("tower_left");
  if (element & 0x20)
    element_names.push_back("tower_right");
  if (element & 0x40)
    element_names.push_back("tower_back");

  return element_names;
}
}

int main(int argc, const char **argv)
{
  try {
    std::string env_ipath;
    std::string opt_ipath;
    std::string ipath;
    std::string opt_configFile_depth;
    std::string opt_modelFile_depth;
    std::string opt_initFile;
    std::string initFile;
    bool displayFeatures = true;
    bool opt_click_allowed = true;
    bool opt_display = true;
    bool useOgre = false;
    bool showOgreConfigDialog = false;
    bool useScanline = false;
    bool computeCovariance = false;
    bool projectionError = false;
    int trackerType_depth = vpMbGenericTracker::DEPTH_DENSE_TRACKER;
#if defined(__mips__) || defined(__mips) || defined(mips) || defined(__MIPS__)
    // To avoid Debian test timeout
    int opt_lastFrame = 5;
#else
    int opt_lastFrame = -1;
#endif
    int disable_castle_faces = 0;

    // Get the visp-images-data package path or VISP_INPUT_IMAGE_PATH
    // environment variable value
    env_ipath = vpIoTools::getViSPImagesDataPath();

    // Set the default input path
    if (!env_ipath.empty())
      ipath = env_ipath;

    // Read the command line options
    if (!getOptions(argc, argv, opt_ipath, opt_configFile_depth, opt_modelFile_depth, opt_initFile, displayFeatures,
                    opt_click_allowed, opt_display, useOgre, showOgreConfigDialog, useScanline, computeCovariance,
                    projectionError, trackerType_depth, opt_lastFrame, disable_castle_faces)) {
      return EXIT_FAILURE;
    }

    // Test if an input path is set
    if (opt_ipath.empty() && env_ipath.empty()) {
      usage(argv[0], NULL);
      std::cerr << std::endl << "ERROR:" << std::endl;
      std::cerr << "  Use -i <visp image path> option or set VISP_INPUT_IMAGE_PATH " << std::endl
                << "  environment variable to specify the location of the " << std::endl
                << "  image path where test images are located." << std::endl
                << std::endl;

      return EXIT_FAILURE;
    }

    // Get the option values
    ipath = vpIoTools::createFilePath(!opt_ipath.empty() ? opt_ipath : env_ipath, "mbt-depth/castel/castel");

    std::string dir_path = vpIoTools::createFilePath(!opt_ipath.empty() ? opt_ipath : env_ipath, "mbt-depth");
    if (!vpIoTools::checkDirectory(dir_path)) {
      std::cerr << "ViSP-images does not contain the folder: " << dir_path << "!" << std::endl;
      return EXIT_SUCCESS;
    }

    std::string configFile_depth;
    if (!opt_configFile_depth.empty())
      configFile_depth = opt_configFile_depth;
    else
      configFile_depth =
          vpIoTools::createFilePath(!opt_ipath.empty() ? opt_ipath : env_ipath, "mbt-depth/castel/chateau_depth.xml");

    std::string modelFile_depth;
    if (!opt_modelFile_depth.empty())
      modelFile_depth = opt_modelFile_depth;
    else
      modelFile_depth =
          vpIoTools::createFilePath(!opt_ipath.empty() ? opt_ipath : env_ipath, "mbt-depth/castel/chateau.cao");

    std::string vrml_ext = ".wrl";
    bool use_vrml =
        (modelFile_depth.compare(modelFile_depth.length() - vrml_ext.length(), vrml_ext.length(), vrml_ext) == 0);

    if (use_vrml) {
#if defined(VISP_HAVE_COIN3D) && (COIN_MAJOR_VERSION == 2 || COIN_MAJOR_VERSION == 3 || COIN_MAJOR_VERSION == 4)
      std::cout << "use_vrml: " << use_vrml << std::endl;
#else
      std::cerr << "Error: vrml model file is only supported if ViSP is "
                   "build with Coin3D 3rd party"
                << std::endl;
      return EXIT_FAILURE;
#endif
    }

    if (!opt_initFile.empty())
      initFile = opt_initFile;
    else
      initFile = vpIoTools::createFilePath(!opt_ipath.empty() ? opt_ipath : env_ipath, "mbt-depth/castel/chateau.init");

    vpImage<unsigned char> I_depth;
    vpImage<uint16_t> I_depth_raw;
    std::vector<vpColVector> pointcloud;
    unsigned int pointcloud_width, pointcloud_height;
    if (!read_data(0, ipath, I_depth_raw, pointcloud, pointcloud_width, pointcloud_height)) {
      std::cerr << "Cannot open sequence: " << ipath << std::endl;
      return EXIT_FAILURE;
    }

    vpImageConvert::createDepthHistogram(I_depth_raw, I_depth);

// initialise a  display
#if defined VISP_HAVE_X11
    vpDisplayX display;
#elif defined VISP_HAVE_GDI
    vpDisplayGDI display;
#elif defined VISP_HAVE_OPENCV
    vpDisplayOpenCV display;
#elif defined VISP_HAVE_D3D9
    vpDisplayD3D display;
#elif defined VISP_HAVE_GTK
    vpDisplayGTK display;
#else
    opt_display = false;
#endif
    if (opt_display) {
#if (defined VISP_HAVE_DISPLAY)
      display.setDownScalingFactor(vpDisplay::SCALE_AUTO);
      display.init(I_depth, 100, 100, "Depth");
#endif
      vpDisplay::display(I_depth);
      vpDisplay::flush(I_depth);
    }

    // Object pointer to check that inheritance is ok
    vpMbTracker *tracker = new vpMbGenericTracker(1, trackerType_depth);
    vpHomogeneousMatrix cMo;
    vpCameraParameters cam;

    loadConfiguration(tracker, configFile_depth);

    // Display the moving edges, and the Klt points
    tracker->setDisplayFeatures(displayFeatures);

    // Tells if the tracker has to use Ogre3D for visibility tests
    tracker->setOgreVisibilityTest(useOgre);
    if (useOgre)
      tracker->setOgreShowConfigDialog(showOgreConfigDialog);

    // Tells if the tracker has to use the scanline visibility tests
    tracker->setScanLineVisibilityTest(useScanline);

    // Tells if the tracker has to compute the covariance matrix
    tracker->setCovarianceComputation(computeCovariance);

    // Tells if the tracker has to compute the projection error
    tracker->setProjectionErrorComputation(projectionError);

    // Retrieve the camera parameters from the tracker
    dynamic_cast<vpMbGenericTracker *>(tracker)->getCameraParameters(cam);

    // Loop to position the cube
    if (opt_display && opt_click_allowed) {
      while (!vpDisplay::getClick(I_depth, false)) {
        vpDisplay::display(I_depth);
        vpDisplay::displayText(I_depth, 15, 10, "click after positioning the object", vpColor::red);
        vpDisplay::flush(I_depth);
      }
    }

    // Load the 3D model (either a vrml file or a .cao file)
    dynamic_cast<vpMbGenericTracker *>(tracker)->loadModel(modelFile_depth);

    if (opt_display && opt_click_allowed) {
      // Initialise the tracker by clicking on the image
      dynamic_cast<vpMbGenericTracker *>(tracker)->initClick(I_depth, initFile, true);
      dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(cMo);
      // display the 3D model at the given pose
      dynamic_cast<vpMbGenericTracker *>(tracker)->display(I_depth, cMo, cam, vpColor::red);
    } else {
      vpHomogeneousMatrix cMoi(0.04431452054, 0.09294637757, 0.3357760654, -2.677922443, 0.121297639, -0.6028463357);
      dynamic_cast<vpMbGenericTracker *>(tracker)->initFromPose(I_depth, cMoi);
    }

    // track the model
    {
      std::map<std::string, const vpImage<unsigned char> *> mapOfImages;
      std::map<std::string, const std::vector<vpColVector> *> mapOfPointclouds;
      mapOfPointclouds["Camera"] = &pointcloud;
      std::map<std::string, unsigned int> mapOfWidths, mapOfHeights;
      mapOfWidths["Camera"] = pointcloud_width;
      mapOfHeights["Camera"] = pointcloud_height;

      dynamic_cast<vpMbGenericTracker *>(tracker)->track(mapOfImages, mapOfPointclouds, mapOfWidths, mapOfHeights);
    }
    dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(cMo);

    if (opt_display) {
      vpDisplay::flush(I_depth);
    }

    bool quit = false, click = false;
    unsigned int frame_index = 0;
    std::vector<double> time_vec;
    while (read_data(frame_index, ipath, I_depth_raw, pointcloud, pointcloud_width, pointcloud_height) && !quit &&
           (opt_lastFrame > 0 ? (int)frame_index <= opt_lastFrame : true)) {
      vpImageConvert::createDepthHistogram(I_depth_raw, I_depth);

      if (opt_display) {
        vpDisplay::display(I_depth);

        std::stringstream ss;
        ss << "Num frame: " << frame_index;
        vpDisplay::displayText(I_depth, 40, 20, ss.str(), vpColor::red);
      }

      // Test reset the tracker
      if (frame_index == 10) {
        std::cout << "----------Test reset tracker----------" << std::endl;
        if (opt_display) {
          vpDisplay::display(I_depth);
        }

        tracker->resetTracker();

        loadConfiguration(tracker, configFile_depth);
        dynamic_cast<vpMbGenericTracker *>(tracker)->loadModel(modelFile_depth);
        dynamic_cast<vpMbGenericTracker *>(tracker)->setCameraParameters(cam);
        tracker->setOgreVisibilityTest(useOgre);
        tracker->setScanLineVisibilityTest(useScanline);
        tracker->setCovarianceComputation(computeCovariance);
        tracker->setProjectionErrorComputation(projectionError);
        dynamic_cast<vpMbGenericTracker *>(tracker)->initFromPose(I_depth, cMo);
      }

// Test to set an initial pose
#if USE_SMALL_DATASET
      if (frame_index == 20) {
        cMo.buildFrom(0.05319520317, 0.09223511976, 0.3380095812, -2.71438192, 0.07141055397, -0.3810081638);
#else
      if (frame_index == 50) {
        cMo.buildFrom(0.06865933578, 0.09494713501, 0.3260555142, -2.730027451, 0.03498390135, 0.01989831338);
#endif
        std::cout << "Test set pose" << std::endl;
        dynamic_cast<vpMbGenericTracker *>(tracker)->setPose(I_depth, cMo);
      }

#if USE_SMALL_DATASET
      // track the object: stop tracking from frame 15 to 20
      if (frame_index < 15 || frame_index >= 20) {
#else
      // track the object: stop tracking from frame 30 to 50
      if (frame_index < 30 || frame_index >= 50) {
#endif
        std::map<std::string, const vpImage<unsigned char> *> mapOfImages;
        std::map<std::string, const std::vector<vpColVector> *> mapOfPointclouds;
        mapOfPointclouds["Camera"] = &pointcloud;
        std::map<std::string, unsigned int> mapOfWidths, mapOfHeights;
        mapOfWidths["Camera"] = pointcloud_width;
        mapOfHeights["Camera"] = pointcloud_height;

        if (disable_castle_faces) {
          std::vector<std::string> element_names = getCastleElementNames(disable_castle_faces);
          std::cout << "Disable: ";
          for (size_t idx = 0; idx < element_names.size(); idx++) {
            std::cout << element_names[idx];
            if (idx + 1 < element_names.size())
              std::cout << ", ";

            if (trackerType_depth & vpMbGenericTracker::DEPTH_DENSE_TRACKER)
              dynamic_cast<vpMbGenericTracker *>(tracker)->setUseDepthDenseTracking(element_names[idx], false);
            if (trackerType_depth & vpMbGenericTracker::DEPTH_NORMAL_TRACKER)
              dynamic_cast<vpMbGenericTracker *>(tracker)->setUseDepthNormalTracking(element_names[idx], false);
          }
          std::cout << std::endl;
        }

        double t = vpTime::measureTimeMs();
        dynamic_cast<vpMbGenericTracker *>(tracker)->track(mapOfImages, mapOfPointclouds, mapOfWidths, mapOfHeights);
        t = vpTime::measureTimeMs() - t;
        time_vec.push_back(t);

        dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(cMo);

        if (opt_display) {
          // display the 3D model
          dynamic_cast<vpMbGenericTracker *>(tracker)->display(I_depth, cMo, cam, vpColor::darkRed);
          // display the frame
          vpDisplay::displayFrame(I_depth, cMo, cam, 0.05);
          // computation time
          std::stringstream ss;
          ss << "Computation time: " << t << " ms";
          vpDisplay::displayText(I_depth, 60, 20, ss.str(), vpColor::red);
          // nb features
          ss.str("");
          ss << "nb features: " << tracker->getError().getRows();
          vpDisplay::displayText(I_depth, 80, 20, ss.str(), vpColor::red);
        }
      }

      if (opt_click_allowed) {
        vpDisplay::displayText(I_depth, 10, 10, "Click to quit", vpColor::red);
        vpMouseButton::vpMouseButtonType button;
        if (vpDisplay::getClick(I_depth, button, click)) {
          switch (button) {
          case vpMouseButton::button1:
            quit = !click;
            break;

          case vpMouseButton::button3:
            click = !click;
            break;

          default:
            break;
          }
        }
      }

      if (computeCovariance) {
        std::cout << "Covariance matrix: \n" << tracker->getCovarianceMatrix() << std::endl << std::endl;
      }

      if (projectionError) {
        std::cout << "Projection error: " << tracker->getProjectionError() << std::endl << std::endl;
      }

      vpDisplay::flush(I_depth);

      frame_index++;
    }

    std::cout << "\nFinal poses, cMo:\n" << cMo << std::endl;
    std::cout << "\nComputation time, Mean: " << vpMath::getMean(time_vec)
              << " ms ; Median: " << vpMath::getMedian(time_vec) << " ms ; Std: " << vpMath::getStdev(time_vec) << " ms"
              << std::endl;

    if (opt_click_allowed && !quit) {
      vpDisplay::getClick(I_depth);
    }

    delete tracker;
    tracker = NULL;

#if defined(VISP_HAVE_XML2) && USE_XML
    // Cleanup memory allocated by xml library used to parse the xml config
    // file in vpMbGenericTracker::loadConfigFile()
    vpXmlParser::cleanup();
#endif

#if defined(VISP_HAVE_COIN3D) && (COIN_MAJOR_VERSION == 2 || COIN_MAJOR_VERSION == 3 || COIN_MAJOR_VERSION == 4)
    // Cleanup memory allocated by Coin library used to load a vrml model in
    // vpMbGenericTracker::loadModel() We clean only if Coin was used.
    if (use_vrml)
      SoDB::finish();
#endif

    return EXIT_SUCCESS;
  } catch (const vpException &e) {
    std::cout << "Catch an exception: " << e << std::endl;
    return EXIT_FAILURE;
  }
}

#else
int main()
{
  std::cerr << "visp_mbt, visp_gui modules and OpenCV are required to run "
               "this example."
            << std::endl;
  return EXIT_SUCCESS;
}
#endif