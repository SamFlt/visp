#include <visp3/dnn_tracker/vpMegaPoseHybridTracker.h>


void vpMegaPoseHybridTracker::init(const vpImage<vpRGBa> &I, const vpRect &bbox)
{
  m_megaTracker.init(I, bbox);
}
void vpMegaPoseHybridTracker::init(const vpImage<vpRGBa> &I, const vpHomogeneousMatrix &cTo)
{
  m_kpTracker.init(I, cTo);
}

void vpMegaPoseHybridTracker::track(const vpImage<vpRGBa> &I)
{

  m_megaTracker.track(I);
  m_kpTracker.track(I);
  m_cTo = m_megaTracker.getLastPoseEstimate().cTo;
}

void vpMegaPoseHybridTracker::display(vpImage<vpRGBa> &I) const
{
  if (!m_tracking) {
    throw vpException(vpException::notInitialized, "Cannot display: tracker is not initialized or has diverged!");
  }
}


void vpMegaPoseHybridTracker::vpMegaPoseKeyPointTracker::init(const vpImage<vpRGBa> &I, const vpHomogeneousMatrix &cTo)
{
  const vpMegaPoseObjectRenders renders = m_megaposeRendering.getObjectRenders(m_parent->m_parameters.m_objectLabel, cTo,
  { vpMegaPoseObjectRenders::RGB, vpMegaPoseObjectRenders::DEPTH });
  std::vector<cv::KeyPoint> keypoints;
}


void vpMegaPoseHybridTracker::vpMegaPoseKeyPointTracker::track(const vpImage<vpRGBa> &I)
{

}

void vpMegaPoseHybridTracker::vpMegaPoseRawTracker::init(const vpImage<vpRGBa> &I, const vpRect &bbox)
{
  std::lock_guard lock(m_inputMutex);
  m_inputImage = I;
  m_inputDetection = bbox;
  m_newImagePending = true;
  m_inputCond.notify_one();
}
void vpMegaPoseHybridTracker::vpMegaPoseRawTracker::track(const vpImage<vpRGBa> &I)
{

  std::lock_guard lock(m_inputMutex);
  m_inputImage = I;
  m_newImagePending = true;
  m_inputCond.notify_one();
}

void vpMegaPoseHybridTracker::vpMegaPoseRawTracker::trackingThreadFn()
{
  vpImage<vpRGBa> I;
  vpRect rect;
  bool initialized = false;
  std::string objectLabel = m_parent->m_parameters.m_objectLabel;
  while (true) {
    { // Process or wait for new input
      std::unique_lock lock(m_inputMutex);
      // wait until we're notified or newImagePending is true (i.e. we start waiting when an image is already available)
      m_inputCond.wait(lock, [this]() { return m_newImagePending; });
      I = m_inputImage;
      m_newImagePending = false;
      if (!initialized) {
        rect = m_inputDetection;
      }

      lock.unlock(); // Release lock on input mutex
    }
    vpHomogeneousMatrix last_cTo;
    { // Get last result
      std::lock_guard lock(m_resultMutex);
      last_cTo = m_lastEstimate.cTo;
    }

    vpMegaPoseEstimate currentEstimate;
    if (!initialized) {
      std::vector<vpRect> det = { rect };
      std::vector<vpMegaPoseEstimate> estimates = m_megaposeTracking.estimatePoses(I, { objectLabel }, nullptr, 0.f, &det, nullptr, 1);
      currentEstimate = estimates[0];
      if (currentEstimate.score > 0.5) {
        initialized = true;
      }
    }
    else {
      std::vector<vpHomogeneousMatrix> pose = { last_cTo };
      std::vector<vpMegaPoseEstimate> estimates = m_megaposeTracking.estimatePoses(I, { objectLabel }, nullptr, 0.f, nullptr, &pose, 1);
      currentEstimate = estimates[0];
    }

    { // Get last result
      std::lock_guard lock(m_resultMutex);
      m_lastEstimate = currentEstimate;
    }

  }
}
