/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2024 by Inria. All rights reserved.
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
 * See https://visp.inria.fr for more information.
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
 * Example of Hybrid Tracking of MBT and MBT KTL.
 */

/*!
  \example mbtGenericTracking.cpp

  \brief Example of tracking with vpGenericTracker on an image sequence
  containing a cube.
*/

#include <iostream>
#include <visp3/core/vpConfig.h>

#if (defined(VISP_HAVE_MODULE_MBT) && defined(VISP_HAVE_DISPLAY)) &&                                                   \
    (defined(VISP_HAVE_LAPACK) || defined(VISP_HAVE_EIGEN3) || defined(VISP_HAVE_OPENCV))

#include <visp3/core/vpDebug.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/core/vpMath.h>
#include <visp3/gui/vpDisplayFactory.h>
#include <visp3/io/vpImageIo.h>
#include <visp3/io/vpParseArgv.h>
#include <visp3/io/vpVideoReader.h>
#include <visp3/mbt/vpMbGenericTracker.h>

#define GETOPTARGS "x:m:i:n:de:chtfColwvpT:"

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

void usage(const char *name, const char *badparam)
{
#if defined(VISP_HAVE_DATASET)
#if VISP_HAVE_DATASET_VERSION >= 0x030600
  std::string ext("png");
#else
  std::string ext("pgm");
#endif
#else
    // We suppose that the user will download a recent dataset
  std::string ext("png");
#endif

  fprintf(stdout, "\n\
Example of tracking based on the 3D model.\n\
\n\
SYNOPSIS\n\
  %s [-i <test image path>] [-x <config file>]\n\
  [-m <model name>] [-n <initialisation file base name>] [-e <last frame index>]\n\
  [-t] [-c] [-d] [-h] [-f] [-C] [-o] [-w] [-l] [-v] [-p]\n\
  [-T <tracker type>]\n",
    name);

  fprintf(stdout, "\n\
OPTIONS:                                               \n\
  -i <input image path>                                \n\
     Set image input path.\n\
     From this path read images \n\
     \"mbt/cube/image%%04d.%s\". These \n\
     images come from visp-images-x.y.z.tar.gz available \n\
     on the ViSP website.\n\
     Setting the VISP_INPUT_IMAGE_PATH environment\n\
     variable produces the same behavior than using\n\
     this option.\n\
\n\
  -x <config file>                                     \n\
     Set the config file (the xml file) to use.\n\
     The config file is used to specify the parameters of the tracker.\n\
\n\
  -m <model name>                                 \n\
     Specify the name of the file of the model\n\
     The model can either be a vrml model (.wrl) or a .cao file.\n\
\n\
  -e <last frame index>                                 \n\
     Specify the index of the last frame. Once reached, the tracking is stopped\n\
\n\
  -f                                  \n\
     Do not use the vrml model, use the .cao one. These two models are \n\
     equivalent and comes from ViSP-images-x.y.z.tar.gz available on the ViSP\n\
     website. However, the .cao model allows to use the 3d model based tracker \n\
     without Coin.\n\
\n\
  -C                                  \n\
     Track only the cube (not the cylinder). In this case the models files are\n\
     cube.cao or cube.wrl instead of cube_and_cylinder.cao and \n\
     cube_and_cylinder.wrl.\n\
\n\
  -n <initialisation file base name>                                            \n\
     Base name of the initialisation file. The file will be 'base_name'.init .\n\
     This base name is also used for the optional picture specifying where to \n\
     click (a .ppm picture).\n\
\n\
  -t \n\
     Turn off the display of the the moving edges and Klt points. \n\
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
     Set tracker type (<1 (Edge)>, <2 (KLT)>, <3 (EdgeKlt)>).\n\
\n\
  -h \n\
     Print the help.\n\n", ext.c_str());

  if (badparam)
    fprintf(stdout, "\nERROR: Bad parameter [%s]\n", badparam);
}

bool getOptions(int argc, const char **argv, std::string &ipath, std::string &configFile, std::string &modelFile,
  std::string &initFile, long &lastFrame, bool &displayFeatures, bool &click_allowed, bool &display,
  bool &cao3DModel, bool &trackCylinder, bool &useOgre, bool &showOgreConfigDialog, bool &useScanline,
  bool &computeCovariance, bool &projectionError, int &trackerType)
{
  const char *optarg_;
  int c;
  while ((c = vpParseArgv::parse(argc, argv, GETOPTARGS, &optarg_)) > 1) {

    switch (c) {
    case 'e':
      lastFrame = atol(optarg_);
      break;
    case 'i':
      ipath = optarg_;
      break;
    case 'x':
      configFile = optarg_;
      break;
    case 'm':
      modelFile = optarg_;
      break;
    case 'n':
      initFile = optarg_;
      break;
    case 't':
      displayFeatures = false;
      break;
    case 'f':
      cao3DModel = true;
      break;
    case 'c':
      click_allowed = false;
      break;
    case 'd':
      display = false;
      break;
    case 'C':
      trackCylinder = false;
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
      trackerType = atoi(optarg_);
      break;
    case 'h':
      usage(argv[0], nullptr);
      return false;

    default:
      usage(argv[0], optarg_);
      return false;
    }
  }

  if ((c == 1) || (c == -1)) {
    // standalone param or error
    usage(argv[0], nullptr);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << "  Bad argument " << optarg_ << std::endl << std::endl;
    return false;
  }

  return true;
}

int main(int argc, const char **argv)
{
  try {
    std::string env_ipath;
    std::string opt_ipath;
    std::string ipath;
    std::string opt_configFile;
    std::string opt_modelFile;
    std::string modelFile;
    std::string opt_initFile;
    std::string initFile;
    long opt_lastFrame = -1;
    bool displayFeatures = true;
    bool opt_click_allowed = true;
    bool opt_display = true;
    bool cao3DModel = false;
    bool trackCylinder = true;
    bool useOgre = false;
    bool showOgreConfigDialog = false;
    bool useScanline = false;
    bool computeCovariance = false;
    bool projectionError = false;
    int trackerType = vpMbGenericTracker::EDGE_TRACKER;

#if defined(VISP_HAVE_DATASET)
#if VISP_HAVE_DATASET_VERSION >= 0x030600
    std::string ext("png");
#else
    std::string ext("pgm");
#endif
#else
    // We suppose that the user will download a recent dataset
    std::string ext("png");
#endif

    // Get the visp-images-data package path or VISP_INPUT_IMAGE_PATH
    // environment variable value
    env_ipath = vpIoTools::getViSPImagesDataPath();

    // Set the default input path
    if (!env_ipath.empty())
      ipath = env_ipath;

    // Read the command line options
    if (!getOptions(argc, argv, opt_ipath, opt_configFile, opt_modelFile, opt_initFile, opt_lastFrame, displayFeatures,
                    opt_click_allowed, opt_display, cao3DModel, trackCylinder, useOgre, showOgreConfigDialog,
                    useScanline, computeCovariance, projectionError, trackerType)) {
      return EXIT_FAILURE;
    }

    // Test if an input path is set
    if (opt_ipath.empty() && env_ipath.empty()) {
      usage(argv[0], nullptr);
      std::cerr << std::endl << "ERROR:" << std::endl;
      std::cerr << "  Use -i <visp image path> option or set VISP_INPUT_IMAGE_PATH " << std::endl
        << "  environment variable to specify the location of the " << std::endl
        << "  image path where test images are located." << std::endl
        << std::endl;

      return EXIT_FAILURE;
    }

    // Get the option values
    if (!opt_ipath.empty())
      ipath = vpIoTools::createFilePath(opt_ipath, "mbt/cube/image%04d." + ext);
    else
      ipath = vpIoTools::createFilePath(env_ipath, "mbt/cube/image%04d." + ext);

#if defined(VISP_HAVE_PUGIXML)
    std::string configFile;
    if (!opt_configFile.empty())
      configFile = opt_configFile;
    else if (!opt_ipath.empty())
      configFile = vpIoTools::createFilePath(opt_ipath, "mbt/cube.xml");
    else
      configFile = vpIoTools::createFilePath(env_ipath, "mbt/cube.xml");
#endif

    if (!opt_modelFile.empty()) {
      modelFile = opt_modelFile;
    }
    else {
      std::string modelFileCao;
      std::string modelFileWrl;
      if (trackCylinder) {
        modelFileCao = "mbt/cube_and_cylinder.cao";
        modelFileWrl = "mbt/cube_and_cylinder.wrl";
      }
      else {
        modelFileCao = "mbt/cube.cao";
        modelFileWrl = "mbt/cube.wrl";
      }

      if (!opt_ipath.empty()) {
        if (cao3DModel) {
          modelFile = vpIoTools::createFilePath(opt_ipath, modelFileCao);
        }
        else {
#ifdef VISP_HAVE_COIN3D
          modelFile = vpIoTools::createFilePath(opt_ipath, modelFileWrl);
#else
          std::cerr << "Coin is not detected in ViSP. Use the .cao model instead." << std::endl;
          modelFile = vpIoTools::createFilePath(opt_ipath, modelFileCao);
#endif
        }
      }
      else {
        if (cao3DModel) {
          modelFile = vpIoTools::createFilePath(env_ipath, modelFileCao);
        }
        else {
#ifdef VISP_HAVE_COIN3D
          modelFile = vpIoTools::createFilePath(env_ipath, modelFileWrl);
#else
          std::cerr << "Coin is not detected in ViSP. Use the .cao model instead." << std::endl;
          modelFile = vpIoTools::createFilePath(env_ipath, modelFileCao);
#endif
        }
      }
    }

    if (!opt_initFile.empty())
      initFile = opt_initFile;
    else if (!opt_ipath.empty())
      initFile = vpIoTools::createFilePath(opt_ipath, "mbt/cube");
    else
      initFile = vpIoTools::createFilePath(env_ipath, "mbt/cube");

    vpImage<unsigned char> I1, I2;
    vpDisplay *display1 = nullptr, *display2 = nullptr;
    vpVideoReader reader;

    reader.setFileName(ipath);
    try {
      reader.open(I1);
      I2 = I1;
    }
    catch (...) {
      std::cerr << "Cannot open sequence: " << ipath << std::endl;
      return EXIT_FAILURE;
    }

    if (opt_lastFrame > 1 && opt_lastFrame < reader.getLastFrameIndex())
      reader.setLastFrameIndex(opt_lastFrame);

    reader.acquire(I1);
    I2 = I1;

    if (opt_display) {
      // We open a window using either X11, GTK, OpenCV or GDI
      display1 = vpDisplayFactory::allocateDisplay();
      display2 = vpDisplayFactory::allocateDisplay();
      display1->setDownScalingFactor(vpDisplay::SCALE_AUTO);
      display2->setDownScalingFactor(vpDisplay::SCALE_AUTO);
      display1->init(I1, 100, 100, "Test tracking (Left)");
      display2->init(I2, static_cast<int>(I1.getWidth() / vpDisplay::getDownScalingFactor(I1)) + 110, 100, "Test tracking (Right)");

      vpDisplay::display(I1);
      vpDisplay::display(I2);
      vpDisplay::flush(I1);
      vpDisplay::flush(I2);
    }

    // Object pointer to check that inheritance is ok
    vpMbTracker *tracker = new vpMbGenericTracker(2, trackerType);
    vpHomogeneousMatrix c1Mo, c2Mo;
    vpCameraParameters cam1, cam2;

    // Initialise the tracker: camera parameters, moving edge and KLT settings
#if defined(VISP_HAVE_PUGIXML)
    // From the xml file
    dynamic_cast<vpMbGenericTracker *>(tracker)->loadConfigFile(configFile, configFile);
#else
    // By setting the parameters:
    cam1.initPersProjWithoutDistortion(547, 542, 338, 234);
    cam2.initPersProjWithoutDistortion(547, 542, 338, 234);

    vpMe me;
    me.setMaskSize(5);
    me.setMaskNumber(180);
    me.setRange(7);
    me.setLikelihoodThresholdType(vpMe::NORMALIZED_THRESHOLD);
    me.setThreshold(10);
    me.setMu1(0.5);
    me.setMu2(0.5);
    me.setSampleStep(4);

#if defined(VISP_HAVE_MODULE_KLT) && defined(VISP_HAVE_OPENCV) && defined(HAVE_OPENCV_IMGPROC) && defined(HAVE_OPENCV_VIDEO)
    vpKltOpencv klt;
    klt.setMaxFeatures(10000);
    klt.setWindowSize(5);
    klt.setQuality(0.01);
    klt.setMinDistance(5);
    klt.setHarrisFreeParameter(0.01);
    klt.setBlockSize(3);
    klt.setPyramidLevels(3);

    dynamic_cast<vpMbGenericTracker *>(tracker)->setKltOpencv(klt);
    dynamic_cast<vpMbGenericTracker *>(tracker)->setKltMaskBorder(5);
#endif

    dynamic_cast<vpMbGenericTracker *>(tracker)->setCameraParameters(cam1, cam2);
    dynamic_cast<vpMbGenericTracker *>(tracker)->setMovingEdge(me);

    tracker->setAngleAppear(vpMath::rad(65));
    tracker->setAngleDisappear(vpMath::rad(75));

    // Specify the clipping to
    tracker->setNearClippingDistance(0.01);
    tracker->setFarClippingDistance(0.90);
    tracker->setClipping(tracker->getClipping() | vpMbtPolygon::FOV_CLIPPING);
    //   tracker->setClipping(tracker->getClipping() | vpMbtPolygon::LEFT_CLIPPING
    //   | vpMbtPolygon::RIGHT_CLIPPING | vpMbtPolygon::UP_CLIPPING |
    //   vpMbtPolygon::DOWN_CLIPPING); // Equivalent to FOV_CLIPPING
#endif

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
    dynamic_cast<vpMbGenericTracker *>(tracker)->getCameraParameters(cam1, cam2);

    // Loop to position the cube
    if (opt_display && opt_click_allowed) {
      while (!vpDisplay::getClick(I1, false)) {
        vpDisplay::display(I1);
        vpDisplay::displayText(I1, 15, 10, "click after positioning the object", vpColor::red);
        vpDisplay::flush(I1);
      }
    }

    // Load the 3D model (either a vrml file or a .cao file)
    tracker->loadModel(modelFile);

    // Initialise the tracker by clicking on the image
    // This function looks for
    //   - a ./cube/cube.init file that defines the 3d coordinates (in meter,
    //   in the object basis) of the points used for the initialisation
    //   - a ./cube/cube.ppm file to display where the user have to click
    //   (Optional, set by the third parameter)
    if (opt_display && opt_click_allowed) {
      dynamic_cast<vpMbGenericTracker *>(tracker)->initClick(I1, I2, initFile, initFile, true);
      dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(c1Mo, c2Mo);
      // display the 3D model at the given pose
      dynamic_cast<vpMbGenericTracker *>(tracker)->display(I1, I2, c1Mo, c2Mo, cam1, cam2, vpColor::red);
    }
    else {
      vpHomogeneousMatrix c1Moi(0.02044769891, 0.1101505452, 0.5078963719, 2.063603907, 1.110231561, -0.4392789872);
      vpHomogeneousMatrix c2Moi(0.02044769891, 0.1101505452, 0.5078963719, 2.063603907, 1.110231561, -0.4392789872);
      dynamic_cast<vpMbGenericTracker *>(tracker)->initFromPose(I1, I2, c1Moi, c2Moi);
    }

    // track the model
    dynamic_cast<vpMbGenericTracker *>(tracker)->track(I1, I2);
    dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(c1Mo, c2Mo);

    if (opt_display) {
      vpDisplay::flush(I1);
      vpDisplay::flush(I2);
    }

    bool quit = false, click = false;
    while (!reader.end() && !quit) {
      // acquire a new image
      reader.acquire(I1);
      I2 = I1;
      // display the image
      if (opt_display) {
        vpDisplay::display(I1);
        vpDisplay::display(I2);

        std::stringstream ss;
        ss << "Num frame: " << reader.getFrameIndex() << "/" << reader.getLastFrameIndex();
        vpDisplay::displayText(I1, 40, 20, ss.str(), vpColor::red);
      }

      // Test to reset the tracker
      if (reader.getFrameIndex() == reader.getFirstFrameIndex() + 10) {
        std::cout << "----------Test reset tracker----------" << std::endl;
        if (opt_display) {
          vpDisplay::display(I1);
          vpDisplay::display(I2);
        }

        tracker->resetTracker();
#if defined(VISP_HAVE_PUGIXML)
        dynamic_cast<vpMbGenericTracker *>(tracker)->loadConfigFile(configFile, configFile);
#else
        // By setting the parameters:
        cam1.initPersProjWithoutDistortion(547, 542, 338, 234);
        cam2.initPersProjWithoutDistortion(547, 542, 338, 234);

        me.setMaskSize(5);
        me.setMaskNumber(180);
        me.setRange(7);
        me.setLikelihoodThresholdType(vpMe::NORMALIZED_THRESHOLD);
        me.setThreshold(10);
        me.setMu1(0.5);
        me.setMu2(0.5);
        me.setSampleStep(4);

#if defined(VISP_HAVE_MODULE_KLT) && defined(VISP_HAVE_OPENCV) && defined(HAVE_OPENCV_IMGPROC) && defined(HAVE_OPENCV_VIDEO)
        klt.setMaxFeatures(10000);
        klt.setWindowSize(5);
        klt.setQuality(0.01);
        klt.setMinDistance(5);
        klt.setHarrisFreeParameter(0.01);
        klt.setBlockSize(3);
        klt.setPyramidLevels(3);

        dynamic_cast<vpMbGenericTracker *>(tracker)->setKltOpencv(klt);
        dynamic_cast<vpMbGenericTracker *>(tracker)->setKltMaskBorder(5);
#endif

        dynamic_cast<vpMbGenericTracker *>(tracker)->setCameraParameters(cam1, cam2);
        dynamic_cast<vpMbGenericTracker *>(tracker)->setMovingEdge(me);
        tracker->setAngleAppear(vpMath::rad(65));
        tracker->setAngleDisappear(vpMath::rad(75));

        // Specify the clipping to
        tracker->setNearClippingDistance(0.01);
        tracker->setFarClippingDistance(0.90);
        tracker->setClipping(tracker->getClipping() | vpMbtPolygon::FOV_CLIPPING);
        //   tracker->setClipping(tracker->getClipping() | vpMbtPolygon::LEFT_CLIPPING
        //   | vpMbtPolygon::RIGHT_CLIPPING | vpMbtPolygon::UP_CLIPPING |
        //   vpMbtPolygon::DOWN_CLIPPING); // Equivalent to FOV_CLIPPING
#endif
        tracker->loadModel(modelFile);
        dynamic_cast<vpMbGenericTracker *>(tracker)->setCameraParameters(cam1, cam2);
        tracker->setOgreVisibilityTest(useOgre);
        tracker->setScanLineVisibilityTest(useScanline);
        tracker->setCovarianceComputation(computeCovariance);
        tracker->setProjectionErrorComputation(projectionError);
        dynamic_cast<vpMbGenericTracker *>(tracker)->initFromPose(I1, I2, c1Mo, c2Mo);
      }

      // Test to set an initial pose
      if (reader.getFrameIndex() == reader.getFirstFrameIndex() + 50) {
        c1Mo.buildFrom(0.0439540832, 0.0845870108, 0.5477322481, 2.179498458, 0.8611798108, -0.3491961946);
        c2Mo.buildFrom(0.0439540832, 0.0845870108, 0.5477322481, 2.179498458, 0.8611798108, -0.3491961946);
        std::cout << "Test set pose" << std::endl;
        dynamic_cast<vpMbGenericTracker *>(tracker)->setPose(I1, I2, c1Mo, c2Mo);
      }

      // track the object: stop tracking from frame 40 to 50
      if (reader.getFrameIndex() - reader.getFirstFrameIndex() < 40 ||
        reader.getFrameIndex() - reader.getFirstFrameIndex() >= 50) {
        dynamic_cast<vpMbGenericTracker *>(tracker)->track(I1, I2);
        dynamic_cast<vpMbGenericTracker *>(tracker)->getPose(c1Mo, c2Mo);
        if (opt_display) {
          // display the 3D model
          dynamic_cast<vpMbGenericTracker *>(tracker)->display(I1, I2, c1Mo, c2Mo, cam1, cam2, vpColor::darkRed);
          // display the frame
          vpDisplay::displayFrame(I1, c1Mo, cam1, 0.05);
          vpDisplay::displayFrame(I2, c2Mo, cam2, 0.05);
        }
      }

      if (opt_click_allowed && opt_display) {
        vpDisplay::displayText(I1, 20, 20, "Click to quit", vpColor::red);
        vpMouseButton::vpMouseButtonType button;
        if (vpDisplay::getClick(I1, button, click)) {
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

      if (opt_display) {
        vpDisplay::flush(I1);
        vpDisplay::flush(I2);
      }
    }

    std::cout << "Reached last frame: " << reader.getFrameIndex() << std::endl;
    std::cout << "\nFinal poses, c1Mo:\n" << c1Mo << "\nc2Mo:\n" << c2Mo << std::endl;

    if (opt_click_allowed && !quit) {
      vpDisplay::getClick(I1);
    }
    reader.close();

    delete tracker;
    tracker = nullptr;

    if (display1) {
      delete display1;
    }
    if (display2) {
      delete display2;
    }
    return EXIT_SUCCESS;
  }
  catch (const vpException &e) {
    std::cout << "Catch an exception: " << e << std::endl;
    return EXIT_FAILURE;
  }
}

#elif !(defined(VISP_HAVE_MODULE_MBT) && defined(VISP_HAVE_DISPLAY))
int main()
{
  std::cout << "Cannot run this example: visp_mbt, visp_gui modules are required." << std::endl;
  return EXIT_SUCCESS;
}
#else
int main()
{
  std::cout << "Cannot run this example: install Lapack, Eigen3 or OpenCV" << std::endl;
  return EXIT_SUCCESS;
}
#endif
