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
  if (!m_tracking) {
    throw vpException(vpException::notInitialized, "Tracker is not initialized or has diverged!");
  }
}

void vpMegaPoseHybridTracker::display(vpImage<vpRGBa> &I) const
{
  if (!m_tracking) {
    throw vpException(vpException::notInitialized, "Cannot display: tracker is not initialized or has diverged!");
  }
}


void vpMegaPoseHybridTracker::vpMegaPoseKeyPointTracker::init(const vpImage<vpRGBa> &I, const vpHomogeneousMatrix &cTo)
{

}

void vpMegaPoseHybridTracker::vpMegaPoseRawTracker::init(const vpImage<vpRGBa> &I, const vpRect &bbox)
{

}
