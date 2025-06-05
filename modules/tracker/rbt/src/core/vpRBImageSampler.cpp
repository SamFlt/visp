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
  unsigned int step = static_cast<unsigned int>(round(coverage1d));

  return std::make_pair(step, step);
}
