#ifndef _vpMegaPoseHybridTracker_h_
#define _vpMegaPoseHybridTracker_h_
#include <condition_variable>
#include <thread>

#include <visp3/dnn_tracker/vpMegaPose.h>

class VISP_EXPORT vpMegaPoseHybridParams
{

public:
  vpMegaPoseHybridParams(const std::string host, int port,
   vpCameraParameters cam, unsigned int height, unsigned width,
   const std::string &label)
    : m_host(host), m_port(port), m_cam(cam), m_height(height), m_width(width), m_objectLabel(label)
  { }

  vpMegaPose makeMegaPoseConnection() const
  {
    return vpMegaPose(m_host, m_port, m_cam, m_height, m_width);
  }

  std::string m_host;
  int m_port;
  vpCameraParameters m_cam;
  unsigned m_height, m_width;
  std::string m_objectLabel;


};

class VISP_EXPORT vpMegaPoseHybridTracker
{
public:
  vpMegaPoseHybridTracker(const vpMegaPoseHybridParams &parameters) :
    m_parameters(parameters), m_kpTracker(this, parameters),
    m_megaTracker(this, parameters), m_cTo(), m_tracking(false)
  { }

  void init(const vpImage<vpRGBa> &I, const vpRect &bbox);
  void init(const vpImage<vpRGBa> &I, const vpHomogeneousMatrix &cTo);

  void track(const vpImage<vpRGBa> &I);

  void display(vpImage<vpRGBa> &I) const;

  vpHomogeneousMatrix getPose() const { return m_cTo; }

  bool isTracking() const { return m_tracking; }

private:

  /**
 * @brief Class that extracts and tracks keypoints from renders provided by megapose
 *
 * This class relies on two components:
 *  - The MegaPose renderings, which return an RGB and depth representations.
 *  - vpKeyPoint, which wraps around OpenCV and provides an easier to extract and match keypoints, as well as leverage
 *  2D-3D correspondence to estimate the pose
 *
 * The goal of this tracker is to provide tracking estimates in between megapose calls.
 * This tracker should run at a higher framerate than vpMegaPoseRawTracker.
 *
 */
  class vpMegaPoseKeyPointTracker
  {
  public:
    vpMegaPoseKeyPointTracker(vpMegaPoseHybridTracker *parent, const vpMegaPoseHybridParams &parameters) :
      m_megaposeRendering(std::move(parameters.makeMegaPoseConnection())), m_parent(parent)
    { }

    void init(const vpImage<vpRGBa> &I, const vpHomogeneousMatrix &cTo);
    void track(const vpImage<vpRGBa> &I);
    void onMegaTrackerNewPoseEstimation();
    void onMegaTrackerFailure();


    vpMegaPose m_megaposeRendering; //! Connection to megapose, used for rendering images and extracting reference keypoints
    vpMegaPoseHybridTracker *m_parent;
  };

  class vpMegaPoseRawTracker
  {
  public:
    vpMegaPoseRawTracker(vpMegaPoseHybridTracker *parent, const vpMegaPoseHybridParams &parameters) :
      m_megaposeTracking(std::move(parameters.makeMegaPoseConnection())), m_parent(parent), m_newImagePending(false),
      m_trackingThread(&vpMegaPoseRawTracker::trackingThreadFn, this)
    {


    }

    void init(const vpImage<vpRGBa> &I, const vpRect &bbox);
    void track(const vpImage<vpRGBa> &I);
    void onKeyPointTrackerNewPoseEstimation();
    void onKeyPointTrackerFailure();
    void trackingThreadFn();
    bool diverged()
    {
      std::lock_guard lock(m_resultMutex);
      return m_lastEstimate.score < 0.5;
    }
  private:
    vpMegaPose m_megaposeTracking; //! Connection to MegaPose, used to retrieve the raw MegaPose tracking results
    vpMegaPoseHybridTracker *m_parent;
    vpImage<vpRGBa> m_inputImage;
    vpRect m_inputDetection;
    bool m_newImagePending;
    std::mutex m_inputMutex, m_resultMutex;
    std::condition_variable m_inputCond, m_resultCond;
    std::thread m_trackingThread;

    vpMegaPoseEstimate m_lastEstimate; //! Last estimate provided by megapose
  };
  vpMegaPoseHybridParams m_parameters;
  vpMegaPoseKeyPointTracker m_kpTracker;
  vpMegaPoseRawTracker m_megaTracker;
  vpHomogeneousMatrix m_cTo; //! Current object pose estimation in the frame of the camera
  bool m_tracking; //! Whether we are currently tracking

};


#endif
