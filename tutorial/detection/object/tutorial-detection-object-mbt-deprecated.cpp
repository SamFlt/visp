//! \example tutorial-detection-object-mbt-deprecated.cpp
#include <visp3/core/vpConfig.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/gui/vpDisplayFactory.h>
#include <visp3/io/vpVideoReader.h>
#include <visp3/mbt/vpMbEdgeTracker.h>
#include <visp3/vision/vpKeyPoint.h>

int main(int argc, char **argv)
{
#if defined(VISP_HAVE_OPENCV) && defined(HAVE_OPENCV_IMGPROC) && defined(VISP_HAVE_DISPLAY) && \
  (((VISP_HAVE_OPENCV_VERSION < 0x050000) && defined(HAVE_OPENCV_CALIB3D) && (defined(HAVE_OPENCV_FEATURES2D) || defined(HAVE_OPENCV_XFEATURES2D))) || \
  ((VISP_HAVE_OPENCV_VERSION >= 0x050000) && defined(HAVE_OPENCV_3D) && defined(HAVE_OPENCV_FEATURES)))

#ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
#endif
  //! [MBT code]
#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
  std::shared_ptr<vpDisplay> display;
#else
  vpDisplay *display = nullptr;
#endif
  try {
    std::string videoname = "teabox.mp4";

    for (int i = 1; i < argc; i++) {
      if (std::string(argv[i]) == "--name" && i + 1 < argc) {
        videoname = std::string(argv[++i]);
      }
      else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
        std::cout << "\nUsage: " << argv[0]
          << " [--name <video name>]"
          << "[--help] [-h]\n" << std::endl;
        return EXIT_SUCCESS;
      }
    }
    std::string parentname = vpIoTools::getParent(videoname);
    std::string objectname = vpIoTools::getNameWE(videoname);

    if (!parentname.empty())
      objectname = parentname + "/" + objectname;

    std::cout << "Video name: " << videoname << std::endl;
    std::cout << "Tracker requested config files: " << objectname << ".[init,"
      << "xml,"
      << "cao or wrl]" << std::endl;
    std::cout << "Tracker optional config files: " << objectname << ".[ppm]" << std::endl;

    vpImage<unsigned char> I;
    vpCameraParameters cam;
    vpHomogeneousMatrix cMo;

    vpVideoReader g;
    g.setFileName(videoname);
    g.open(I);

#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
    display = vpDisplayFactory::createDisplay(I, 100, 100, "Model-based edge tracker");
#else
    display = vpDisplayFactory::allocateDisplay(I, 100, 100, "Model-based edge tracker");
#endif

    vpMbEdgeTracker tracker;
    bool usexml = false;
#if defined(VISP_HAVE_PUGIXML)
    if (vpIoTools::checkFilename(objectname + ".xml")) {
      tracker.loadConfigFile(objectname + ".xml");
      tracker.getCameraParameters(cam);
      usexml = true;
    }
#endif
    if (!usexml) {
      vpMe me;
      me.setMaskSize(5);
      me.setMaskNumber(180);
      me.setRange(8);
      me.setLikelihoodThresholdType(vpMe::NORMALIZED_THRESHOLD);
      me.setThreshold(20);
      me.setMu1(0.5);
      me.setMu2(0.5);
      me.setSampleStep(4);
      me.setNbTotalSample(250);
      tracker.setMovingEdge(me);
      cam.initPersProjWithoutDistortion(839, 839, 325, 243);
      tracker.setCameraParameters(cam);
      tracker.setAngleAppear(vpMath::rad(70));
      tracker.setAngleDisappear(vpMath::rad(80));
      tracker.setNearClippingDistance(0.1);
      tracker.setFarClippingDistance(100.0);
      tracker.setClipping(tracker.getClipping() | vpMbtPolygon::FOV_CLIPPING);
    }

    tracker.setOgreVisibilityTest(false);
    if (vpIoTools::checkFilename(objectname + ".cao"))
      tracker.loadModel(objectname + ".cao");
    else if (vpIoTools::checkFilename(objectname + ".wrl"))
      tracker.loadModel(objectname + ".wrl");
    tracker.setDisplayFeatures(true);
    tracker.initClick(I, objectname + ".init", true);
    tracker.track(I);
    //! [MBT code]

    //! [Keypoint selection]
#if ((VISP_HAVE_OPENCV_VERSION < 0x050000) && defined(HAVE_OPENCV_XFEATURES2D)) || ((VISP_HAVE_OPENCV_VERSION >= 0x050000) && defined(HAVE_OPENCV_FEATURES))
    std::string detectorName = "SIFT";
    std::string extractorName = "SIFT";
    std::string matcherName = "BruteForce";
    std::string configurationFile = "detection-config-SIFT.xml";
#elif ((VISP_HAVE_OPENCV_VERSION < 0x050000) && defined(HAVE_OPENCV_FEATURES2D)) || ((VISP_HAVE_OPENCV_VERSION >= 0x050000) && defined(HAVE_OPENCV_FEATURES))
    std::string detectorName = "FAST";
    std::string extractorName = "ORB";
    std::string matcherName = "BruteForce-Hamming";
    std::string configurationFile = "detection-config.xml";
#endif
    //! [Keypoint selection]

    //! [Keypoint declaration]
    vpKeyPoint keypoint_learning;
    //! [Keypoint declaration]
    if (usexml) {
      //! [Keypoint xml config]
      keypoint_learning.loadConfigFile(configurationFile);
      //! [Keypoint xml config]
    }
    else {
      //! [Keypoint code config]
      keypoint_learning.setDetector(detectorName);
      keypoint_learning.setExtractor(extractorName);
      keypoint_learning.setMatcher(matcherName);
      //! [Keypoint code config]
    }

    //! [Keypoints reference detection]
    std::vector<cv::KeyPoint> trainKeyPoints;
    double elapsedTime;
    keypoint_learning.detect(I, trainKeyPoints, elapsedTime);
    //! [Keypoints reference detection]

    //! [Keypoints selection on faces]
    std::vector<vpPolygon> polygons;
    std::vector<std::vector<vpPoint> > roisPt;
    std::pair<std::vector<vpPolygon>, std::vector<std::vector<vpPoint> > > pair = tracker.getPolygonFaces(false);
    polygons = pair.first;
    roisPt = pair.second;

    std::vector<cv::Point3f> points3f;
    tracker.getPose(cMo);
    vpKeyPoint::compute3DForPointsInPolygons(cMo, cam, trainKeyPoints, polygons, roisPt, points3f);
    //! [Keypoints selection on faces]

    //! [Keypoints build reference]
    keypoint_learning.buildReference(I, trainKeyPoints, points3f);
    //! [Keypoints build reference]

    //! [Save learning data]
    keypoint_learning.saveLearningData("teabox_learning_data.bin", true);
    //! [Save learning data]

    //! [Display reference keypoints]
    vpDisplay::display(I);
    for (std::vector<cv::KeyPoint>::const_iterator it = trainKeyPoints.begin(); it != trainKeyPoints.end(); ++it) {
      vpDisplay::displayCross(I, static_cast<int>(it->pt.y), static_cast<int>(it->pt.x), 4, vpColor::red);
    }
    vpDisplay::displayText(I, 10, 10, "Learning step: keypoints are detected on visible teabox faces", vpColor::red);
    vpDisplay::displayText(I, 30, 10, "Click to continue with detection...", vpColor::red);
    vpDisplay::flush(I);
    vpDisplay::getClick(I, true);
    //! [Display reference keypoints]

    //! [Init keypoint detection]
    vpKeyPoint keypoint_detection;
    if (usexml) {
      keypoint_detection.loadConfigFile(configurationFile);
    }
    else {
      keypoint_detection.setDetector(detectorName);
      keypoint_detection.setExtractor(extractorName);
      keypoint_detection.setMatcher(matcherName);
      keypoint_detection.setFilterMatchingType(vpKeyPoint::ratioDistanceThreshold);
      keypoint_detection.setMatchingRatioThreshold(0.8);
      keypoint_detection.setUseRansacVVS(true);
      keypoint_detection.setUseRansacConsensusPercentage(true);
      keypoint_detection.setRansacConsensusPercentage(20.0);
      keypoint_detection.setRansacIteration(200);
      keypoint_detection.setRansacThreshold(0.005);
    }
    //! [Init keypoint detection]

    //! [Load teabox learning data]
    keypoint_detection.loadLearningData("teabox_learning_data.bin", true);
    //! [Load teabox learning data]

    double error;
    bool click_done = false;

    while (!g.end()) {
      g.acquire(I);
      vpDisplay::display(I);

      vpDisplay::displayText(I, 10, 10, "Detection and localization in process...", vpColor::red);

      //! [Matching and pose estimation]
      if (keypoint_detection.matchPoint(I, cam, cMo, error, elapsedTime)) {
        //! [Matching and pose estimation]

        //! [Tracker set pose]
        tracker.setPose(I, cMo);
        //! [Tracker set pose]
        //! [Display]
        tracker.display(I, cMo, cam, vpColor::red, 2);
        vpDisplay::displayFrame(I, cMo, cam, 0.025, vpColor::none, 3);
        //! [Display]
      }

      vpDisplay::displayText(I, 30, 10, "A click to exit.", vpColor::red);
      vpDisplay::flush(I);
      if (vpDisplay::getClick(I, false)) {
        click_done = true;
        break;
      }
    }
    if (!click_done)
      vpDisplay::getClick(I);
  }
  catch (const vpException &e) {
    std::cout << "Catch an exception: " << e << std::endl;
  }

#if (VISP_CXX_STANDARD < VISP_CXX_STANDARD_11)
  if (display != nullptr) {
    delete display;
  }
#endif
#else
  (void)argc;
  (void)argv;
  std::cout << "Install OpenCV and rebuild ViSP to use this example." << std::endl;
#endif

  return EXIT_SUCCESS;
}
