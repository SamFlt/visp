#include <visp3/rbt/vpRBImageSampler.h>

#include <visp3/rbt/vpRBFeatureTrackerInput.h>

#if defined(VISP_HAVE_NLOHMANN_JSON)
std::shared_ptr<vpRBImageSampler> vpRBImageSampler::loadSampler(const nlohmann::json &j)
{
  if (j.is_number_integer()) {
    return std::make_shared<vpRBFixedStepImageSampler>(j); // default value
  }
  else if (j.is_object()) {
    if (!j.contains("type")) {
      throw vpException(vpException::badValue, "Expected type key when parsing Sampler json");
    }
    const std::string typeKey = j["type"];
    if (typeKey == "fixed") {
      return std::make_shared<vpRBFixedStepImageSampler>(j.at("step"));
    }
    else if (typeKey == "boundingBoxCoverage") {
      return std::make_shared<vpRBBoundingBoxImageSampler>(j.at("coverage"));
    }
    else if (typeKey == "target") {
      return std::make_shared<vpRBTargetImageSampler>(j.at("numPixels"));
    }
    else {
      throw vpException(vpException::badValue, "Unexpected type %s when parsing sampler json", typeKey.c_str());
    }
  }
  else {
    throw vpException(vpException::badValue, "Unexpected sample type when parsing json");
  }
}
#endif

vpRBFixedStepImageSampler::vpRBFixedStepImageSampler(unsigned int step) : m_step(step)
{ }

std::pair<unsigned int, unsigned int> vpRBFixedStepImageSampler::getSampleSteps(const vpRBFeatureTrackerInput &)
{
  return std::make_pair(m_step, m_step);
}


vpRBBoundingBoxImageSampler::vpRBBoundingBoxImageSampler(double coverage) : m_coverage(coverage)
{ }

std::pair<unsigned int, unsigned int> vpRBBoundingBoxImageSampler::getSampleSteps(const vpRBFeatureTrackerInput &)
{
  double coverage1d = sqrt(m_coverage);
  unsigned int step = static_cast<unsigned int>(round(1.0 / coverage1d));

  return std::make_pair(step, step);
}

vpRBTargetImageSampler::vpRBTargetImageSampler(unsigned int target) : m_target(target)
{ }

std::pair<unsigned int, unsigned int> vpRBTargetImageSampler::getSampleSteps(const vpRBFeatureTrackerInput &frame)
{
  vpRect bb = frame.renders.boundingBox;
  double width = bb.getWidth(), height = bb.getHeight();
  int stepH = 1, stepW = 1;
  double bestSol = width * height - m_target;
  while (true) {
    double wSol = abs((width / (stepW + 1)) * (height / stepH) - static_cast<int>(m_target));
    double hSol = abs((width / stepW) * (height / (stepH + 1)) - static_cast<int>(m_target));

    if (wSol >= bestSol && hSol >= bestSol) {
      break;
    }

    if (wSol < hSol) {
      bestSol = wSol;
      stepW++;
    }
    else if (hSol <= wSol) {
      bestSol = hSol;
      stepH++;
    }
  }
  return std::make_pair(stepH, stepW);
}
