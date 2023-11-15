/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2023 by Inria. All rights reserved.
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
 */

#ifndef _vpCircleHoughTransform_h_
#define _vpCircleHoughTransform_h_

 // System includes
#include <utility>
#include <vector>

// ViSP includes
#include <visp3/core/vpConfig.h>
#include <visp3/core/vpCannyEdgeDetection.h>
#include <visp3/core/vpImage.h>
#include <visp3/core/vpImageCircle.h>
#include <visp3/core/vpImageDraw.h>
#include <visp3/core/vpImageFilter.h>
#include <visp3/core/vpImagePoint.h>
#include <visp3/core/vpMatrix.h>
#include <visp3/core/vpRect.h>

// 3rd parties inclue
#ifdef VISP_HAVE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
//! json namespace shortcut
using json = nlohmann::json;
#endif

/**
 * \ingroup group_hough_transform
 * \brief Class that permits to detect 2D circles in a image using
 * the gradient-based Circle Hough transform.
 * Please find more information on the algorithm
 * [here](https://theailearner.com/tag/hough-gradient-method-opencv/)
 *
 */
class VISP_EXPORT vpCircleHoughTransform
{
public:
  /**
   * \brief Class that gather the algorithm parameters.
   */
  class vpCircleHoughTransformParameters
  {
  private:
    // // Filtering + gradient operators to use
    vpImageFilter::vpCannyFilteringAndGradientType m_filteringAndGradientType; /*!< Permits to choose the filtering +
                                                                                    gradient operators to use.*/

    // // Gaussian smoothing attributes
    int m_gaussianKernelSize; /*!< Size of the Gaussian filter kernel used to smooth the input image.
                                   Must be an odd number.*/
    float m_gaussianStdev;   /*!< Standard deviation of the Gaussian filter.*/

    // // Gradient computation attributes
    int m_gradientFilterKernelSize; /*!< Size of the Sobel or Scharr kernels used to compute the gradients. Must be an odd number.*/

    // // Edge detection attributes
    float m_lowerCannyThresh; /*!< The lower threshold for the Canny operator. Values lower than this value are rejected.
                               A negative value makes the algorithm compute the lower threshold automatically.*/
    float m_upperCannyThresh; /*!< The upper threshold for the Canny operator. Only values greater than this value are marked as an edge.
                               A negative value makes the algorithm compute the upper and lower thresholds automatically.*/
    int m_edgeMapFilteringNbIter; /*!< Number of iterations of 8-neighbor connectivity filtering to apply to the edge map*/
    vpImageFilter::vpCannyBackendType m_cannyBackendType; /*!< Permits to choose the backend used to compute the edge map.*/
    float m_lowerCannyThreshRatio; /*!< The ratio of the upper threshold the lower threshold must be equal to.
                                        It is used only if the user asks to compute the Canny thresholds.*/
    float m_upperCannyThreshRatio; /*!< The ratio of pixels whose absolute gradient Gabs is lower or equal to define
                                        the upper threshold. It is used only if the user asks to compute the Canny thresholds.*/

    // // Center candidates computation attributes
    std::pair<int, int> m_centerXlimits; /*!< Minimum and maximum position on the horizontal axis of the center of the circle we want to detect.*/
    std::pair<int, int> m_centerYlimits; /*!< Minimum and maximum position on the vertical axis of the center of the circle we want to detect.*/
    unsigned int m_minRadius; /*!< Minimum radius of the circles we want to detect.*/
    unsigned int m_maxRadius; /*!< Maximum radius of the circles we want to detect.*/
    int m_dilatationNbIter; /*!< Number of times dilatation is performed to detect the maximum number of votes for the center candidates.*/
    float m_centerThresh;  /*!< Minimum number of votes a point must exceed to be considered as center candidate.*/

    // // Circle candidates computation attributes
    float m_circleProbaThresh;  /*!< Probability threshold in order to keep a circle candidate.*/
    float m_circlePerfectness; /*!< The scalar product radius RC_ij . gradient(Ep_j) >=  m_circlePerfectness * || RC_ij || * || gradient(Ep_j) || to add a vote for the radius RC_ij. */

    // // Circle candidates merging attributes
    float m_centerMinDist; /*!< Maximum distance between two circle candidates centers to consider merging them.*/
    float m_mergingRadiusDiffThresh; /*!< Maximum radius difference between two circle candidates to consider merging them.*/

    friend class vpCircleHoughTransform;
  public:
    /**
     * \brief Construct a new vpCircleHoughTransformParameters object with default parameters.
     */
    vpCircleHoughTransformParameters()
      : m_filteringAndGradientType(vpImageFilter::CANNY_GBLUR_SOBEL_FILTERING)
      , m_gaussianKernelSize(5)
      , m_gaussianStdev(1.f)
      , m_gradientFilterKernelSize(3)
      , m_lowerCannyThresh(-1.f)
      , m_upperCannyThresh(-1.f)
      , m_edgeMapFilteringNbIter(1)
      , m_cannyBackendType(vpImageFilter::CANNY_OPENCV_BACKEND)
      , m_lowerCannyThreshRatio(0.6f)
      , m_upperCannyThreshRatio(0.8f)
      , m_centerXlimits(std::pair<int, int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()))
      , m_centerYlimits(std::pair<int, int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()))
      , m_minRadius(0)
      , m_maxRadius(1000)
      , m_dilatationNbIter(1)
      , m_centerThresh(50.f)
      , m_circleProbaThresh(0.9f)
      , m_circlePerfectness(0.9f)
      , m_centerMinDist(15.f)
      , m_mergingRadiusDiffThresh(1.5f * m_centerMinDist)
    {

    }

    /**
     * \brief Construct a new vpCircleHoughTransformParameters object.
     *
     * \param[in] gaussianKernelSize Size of the Gaussian filter kernel used to smooth the input image. Must be an odd number.
     * \param[in] gaussianStdev Standard deviation of the Gaussian filter.
     * \param[in] gradientFilterKernelSize Size of the Sobel or Scharr kernels used to compute the gradients. Must be an odd number.
     * \param[in] lowerCannyThresh The lower threshold for the Canny operator. Values lower than this value are rejected.
                          A negative value makes the algorithm compute this threshold and the lower one automatically.
     * \param[in] upperCannyThresh The upper threshold for the Canny operator. Only values greater than this value are marked as an edge.
                          A negative value makes the algorithm compute this threshold and the lower one automatically.
     * \param[in] edgeMapFilterNbIter Number of 8-neighbor connectivity filtering iterations to apply to the edge map.
     * \param[in] centerXlimits Minimum and maximum position on the horizontal axis of the center of the circle we want to detect.
     * \param[in] centerYlimits Minimum and maximum position on the vertical axis of the center of the circle we want to detect.
     * \param[in] minRadius Minimum radius of the circles we want to detect.
     * \param[in] maxRadius Maximum radius of the circles we want to detect.
     * \param[in] dilatationNbIter Number of times dilatation is performed to detect the maximum number of votes for the center candidates
     * \param[in] centerThresh Minimum number of votes a point must exceed to be considered as center candidate.
     * \param[in] circleProbabilityThresh Probability threshold in order to keep a circle candidate.
     * \param[in] circlePerfectness The scalar product radius RC_ij . gradient(Ep_j) >=  m_circlePerfectness * || RC_ij || * || gradient(Ep_j) || to add a vote for the radius RC_ij.
     * \param[in] centerMinDistThresh  Two circle candidates whose centers are closer than this threshold are considered for merging.
     * \param[in] mergingRadiusDiffThresh Maximum radius difference between two circle candidates to consider merging them.
     * \param[in] filteringAndGradientMethod The choice of the filter and gradient operator to apply before the edge
     * detection step.
     * \param[in] backendType Permits to choose the backend used to compute the edge map.
     * \param[in] lowerCannyThreshRatio If the thresholds must be computed,the lower threshold will be equal to the upper
     * threshold times \b lowerThresholdRatio .
     * \param[in] upperCannyThreshRatio If the thresholds must be computed,the upper threshold will be equal to the value
     * such as the number of pixels of the image times \b upperThresholdRatio have an absolute gradient lower than the
     * upper threshold.
     */
    vpCircleHoughTransformParameters(
        const int &gaussianKernelSize
      , const float &gaussianStdev
      , const int &gradientFilterKernelSize
      , const float &lowerCannyThresh
      , const float &upperCannyThresh
      , const int &edgeMapFilterNbIter
      , const std::pair<int, int> &centerXlimits
      , const std::pair<int, int> &centerYlimits
      , const unsigned int &minRadius
      , const unsigned int &maxRadius
      , const int &dilatationNbIter
      , const float &centerThresh
      , const float &circleProbabilityThresh
      , const float &circlePerfectness
      , const float &centerMinDistThresh
      , const float &mergingRadiusDiffThresh
      , const vpImageFilter::vpCannyFilteringAndGradientType &filteringAndGradientMethod = vpImageFilter::CANNY_GBLUR_SOBEL_FILTERING
      , const vpImageFilter::vpCannyBackendType &backendType = vpImageFilter::CANNY_OPENCV_BACKEND
      , const float &lowerCannyThreshRatio = 0.6f
      , const float &upperCannyThreshRatio = 0.8f
    )
      : m_filteringAndGradientType(filteringAndGradientMethod)
      , m_gaussianKernelSize(gaussianKernelSize)
      , m_gaussianStdev(gaussianStdev)
      , m_gradientFilterKernelSize(gradientFilterKernelSize)
      , m_lowerCannyThresh(lowerCannyThresh)
      , m_upperCannyThresh(upperCannyThresh)
      , m_edgeMapFilteringNbIter(edgeMapFilterNbIter)
      , m_cannyBackendType(backendType)
      , m_lowerCannyThreshRatio(lowerCannyThreshRatio)
      , m_upperCannyThreshRatio(upperCannyThreshRatio)
      , m_centerXlimits(centerXlimits)
      , m_centerYlimits(centerYlimits)
      , m_minRadius(std::min(minRadius, maxRadius))
      , m_maxRadius(std::max(minRadius, maxRadius))
      , m_dilatationNbIter(dilatationNbIter)
      , m_centerThresh(centerThresh)
      , m_circleProbaThresh(circleProbabilityThresh)
      , m_circlePerfectness(circlePerfectness)
      , m_centerMinDist(centerMinDistThresh)
      , m_mergingRadiusDiffThresh(mergingRadiusDiffThresh)
    { }

    /**
     * Create a string with all the Hough transform parameters.
     */
    std::string toString() const
    {
      std::string txt("Hough Circle Transform Configuration:\n");
      txt += "\tFiltering + gradient operators = " + vpImageFilter::vpCannyFilteringAndGradientTypeToString(m_filteringAndGradientType) + "\n";
      txt += "\tGaussian filter kernel size = " + std::to_string(m_gaussianKernelSize) + "\n";
      txt += "\tGaussian filter standard deviation = " + std::to_string(m_gaussianStdev) + "\n";
      txt += "\tGradient filter kernel size = " + std::to_string(m_gradientFilterKernelSize) + "\n";
      txt += "\tCanny backend = " + vpImageFilter::vpCannyBackendTypeToString(m_cannyBackendType) + "\n";
      txt += "\tCanny edge filter thresholds = [" + std::to_string(m_lowerCannyThresh) + " ; " + std::to_string(m_upperCannyThresh) + "]\n";
      txt += "\tCanny edge filter thresholds ratio (for auto-thresholding) = [" + std::to_string(m_lowerCannyThreshRatio) + " ; " + std::to_string(m_upperCannyThreshRatio) + "]\n";
      txt += "\tEdge map 8-neighbor connectivity filtering number of iterations = " + std::to_string(m_edgeMapFilteringNbIter) + "\n";
      txt += "\tCenter horizontal position limits: min = " + std::to_string(m_centerXlimits.first) + "\tmax = " + std::to_string(m_centerXlimits.second) +"\n";
      txt += "\tCenter vertical position limits: min = " + std::to_string(m_centerYlimits.first) + "\tmax = " + std::to_string(m_centerYlimits.second) +"\n";
      txt += "\tRadius limits: min = " + std::to_string(m_minRadius) + "\tmax = " + std::to_string(m_maxRadius) +"\n";
      txt += "\tNumber of repetitions of the dilatation filter = " + std::to_string(m_dilatationNbIter) + "\n";
      txt += "\tCenters votes threshold = " + std::to_string(m_centerThresh) + "\n";
      txt += "\tCircle probability threshold = " + std::to_string(m_circleProbaThresh) + "\n";
      txt += "\tCircle perfectness threshold = " + std::to_string(m_circlePerfectness) + "\n";
      txt += "\tCenters minimum distance = " + std::to_string(m_centerMinDist) + "\n";
      txt += "\tRadius difference merging threshold = " + std::to_string(m_mergingRadiusDiffThresh) + "\n";
      return txt;
    }

    // // Configuration from files
#ifdef VISP_HAVE_NLOHMANN_JSON
  /**
   * \brief Create a new vpCircleHoughTransformParameters from a JSON file.
   *
   * \param[in] jsonFile The path towards the JSON file.
   * \return vpCircleHoughTransformParameters The corresponding vpCircleHoughTransformParameters object.
   */
    inline static vpCircleHoughTransformParameters createFromJSON(const std::string &jsonFile)
    {
      std::ifstream file(jsonFile);
      if (!file.good()) {
        std::stringstream ss;
        ss << "Problem opening file " << jsonFile << ". Make sure it exists and is readable" << std::endl;
        throw vpException(vpException::ioError, ss.str());
      }
      json j;
      try {
        j = json::parse(file);
      }
      catch (json::parse_error &e) {
        std::stringstream msg;
        msg << "Could not parse JSON file : \n";

        msg << e.what() << std::endl;
        msg << "Byte position of error: " << e.byte;
        throw vpException(vpException::ioError, msg.str());
      }
      vpCircleHoughTransformParameters params = j; // Call from_json(const json& j, vpDetectorDNN& *this) to read json
      file.close();
      return params;
    }

    /**
     * \brief Save the configuration of the detector in a JSON file
     * described by the path \b jsonPath. Throw a \b vpException
     * is the file cannot be created.
     *
     * \param[in] jsonPath The path towards the JSON output file.
     */
    inline void saveConfigurationInJSON(const std::string &jsonPath) const
    {
      std::ofstream file(jsonPath);
      const json j = *this;
      file << j.dump(4);
      file.close();
    }

    /**
     * \brief Read the detector configuration from JSON. All values are optional and if an argument is not present,
     * the default value defined in the constructor is kept
     *
     * \param[in] j : The JSON object, resulting from the parsing of a JSON file.
     * \param[out] params : The circle Hough transform parameters that will be initialized from the JSON data.
     */
    inline friend void from_json(const json &j, vpCircleHoughTransformParameters &params)
    {
      std::string filteringAndGradientName = vpImageFilter::vpCannyFilteringAndGradientTypeToString(params.m_filteringAndGradientType);
      filteringAndGradientName = j.value("filteringAndGradientType", filteringAndGradientName);
      params.m_filteringAndGradientType = vpImageFilter::vpCannyFilteringAndGradientTypeFromString(filteringAndGradientName);

      params.m_gaussianKernelSize = j.value("gaussianKernelSize", params.m_gaussianKernelSize);
      if ((params.m_gaussianKernelSize % 2) != 1) {
        throw vpException(vpException::badValue, "Gaussian Kernel size should be odd.");
      }

      params.m_gaussianStdev = j.value("gaussianStdev", params.m_gaussianStdev);
      if (params.m_gaussianStdev <= 0) {
        throw vpException(vpException::badValue, "Standard deviation should be > 0");
      }

      params.m_gradientFilterKernelSize = j.value("gradientFilterKernelSize", params.m_gradientFilterKernelSize);
      if ((params.m_gradientFilterKernelSize % 2) != 1) {
        throw vpException(vpException::badValue, "Gradient filter kernel (Sobel or Scharr) size should be odd.");
      }

      std::string cannyBackendName = vpImageFilter::vpCannyBackendTypeToString(params.m_cannyBackendType);
      cannyBackendName = j.value("cannyBackendType", cannyBackendName);
      params.m_cannyBackendType = vpImageFilter::vpCannyBackendTypeFromString(cannyBackendName);
      params.m_lowerCannyThresh = j.value("lowerCannyThresh", params.m_lowerCannyThresh);
      params.m_lowerCannyThreshRatio = j.value("lowerThresholdRatio", params.m_lowerCannyThreshRatio);
      params.m_upperCannyThresh = j.value("upperCannyThresh", params.m_upperCannyThresh);
      params.m_upperCannyThreshRatio = j.value("upperThresholdRatio", params.m_upperCannyThreshRatio);
      params.m_edgeMapFilteringNbIter = j.value("edgeMapFilteringNbIter", params.m_edgeMapFilteringNbIter);

      params.m_centerXlimits = j.value("centerXlimits", params.m_centerXlimits);
      params.m_centerYlimits = j.value("centerYlimits", params.m_centerYlimits);
      std::pair<unsigned int, unsigned int> radiusLimits = j.value("radiusLimits", std::pair<unsigned int, unsigned int>(params.m_minRadius, params.m_maxRadius));
      params.m_minRadius = std::min(radiusLimits.first, radiusLimits.second);
      params.m_maxRadius = std::max(radiusLimits.first, radiusLimits.second);

      params.m_dilatationNbIter = j.value("dilatationNbIter", params.m_dilatationNbIter);

      params.m_centerThresh = j.value("centerThresh", params.m_centerThresh);
      if (params.m_centerThresh <= 0) {
        throw vpException(vpException::badValue, "Votes thresholds for center detection must be positive.");
      }

      params.m_circleProbaThresh = j.value("circleProbabilityThreshold", params.m_circleProbaThresh);

      params.m_circlePerfectness = j.value("circlePerfectnessThreshold", params.m_circlePerfectness);

      if (params.m_circlePerfectness <= 0 || params.m_circlePerfectness > 1) {
        throw vpException(vpException::badValue, "Circle perfectness must be in the interval ] 0; 1].");
      }

      params.m_centerMinDist = j.value("centerMinDistance", params.m_centerMinDist);
      if (params.m_centerMinDist <= 0) {
        throw vpException(vpException::badValue, "Centers minimum distance threshold must be positive.");
      }

      params.m_mergingRadiusDiffThresh = j.value("mergingRadiusDiffThresh", params.m_mergingRadiusDiffThresh);
      if (params.m_mergingRadiusDiffThresh <= 0) {
        throw vpException(vpException::badValue, "Radius difference merging threshold must be positive.");
      }
    }

    /**
     * \brief Parse a vpCircleHoughTransform into JSON format.
     *
     * \param[out] j : A JSON parser object.
     * \param[in] params : The circle Hough transform parameters that will be serialized in the json object.
     */
    inline friend void to_json(json &j, const vpCircleHoughTransformParameters &params)
    {
      std::pair<unsigned int, unsigned int> radiusLimits = { params.m_minRadius, params.m_maxRadius };

      j = json {
          {"filteringAndGradientType", vpImageFilter::vpCannyFilteringAndGradientTypeToString(params.m_filteringAndGradientType)},
          {"gaussianKernelSize", params.m_gaussianKernelSize},
          {"gaussianStdev", params.m_gaussianStdev},
          {"gradientFilterKernelSize", params.m_gradientFilterKernelSize},
          {"cannyBackendType", vpImageFilter::vpCannyBackendTypeToString(params.m_cannyBackendType)},
          {"lowerCannyThresh", params.m_lowerCannyThresh},
          {"lowerThresholdRatio", params.m_lowerCannyThreshRatio},
          {"upperCannyThresh", params.m_upperCannyThresh},
          {"upperThresholdRatio", params.m_upperCannyThreshRatio},
          {"edgeMapFilteringNbIter", params.m_edgeMapFilteringNbIter},
          {"centerXlimits", params.m_centerXlimits},
          {"centerYlimits", params.m_centerYlimits},
          {"radiusLimits", radiusLimits},
          {"dilatationNbIter", params.m_dilatationNbIter},
          {"centerThresh", params.m_centerThresh},
          {"circleProbabilityThreshold", params.m_circleProbaThresh},
          {"circlePerfectnessThreshold", params.m_circlePerfectness},
          {"centerMinDistance", params.m_centerMinDist},
          {"mergingRadiusDiffThresh", params.m_mergingRadiusDiffThresh} };
    }
#endif
  };

  /**
   * \brief Construct a new vpCircleHoughTransform object with default parameters.
   */
  vpCircleHoughTransform();

  /**
   * \brief Construct a new vpCircleHoughTransform object
   * from a \b vpCircleHoughTransformParameters object.
   * \param[in] algoParams The parameters of the Circle Hough Transform.
   */
  vpCircleHoughTransform(const vpCircleHoughTransformParameters &algoParams);

  /**
   * \brief Destroy the vp Circle Hough Transform object
   */
  virtual ~vpCircleHoughTransform();

  /** @name  Detection methods */
  //@{
#ifdef HAVE_OPENCV_CORE
  /**
   * \brief Perform Circle Hough Transform to detect the circles in an OpenCV image.
   *
   * \param[in] I The input gray scale image.
   * \return std::vector<vpImageCircle> The list of 2D circles detected in the image.
   */
  std::vector<vpImageCircle> detect(const cv::Mat &cv_I);
#endif

  /**
   * \brief Convert the input image in a gray-scale image and then
   * perform Circle Hough Transform to detect the circles in it
   *
   * \param[in] I The input color image.
   * \return std::vector<vpImageCircle> The list of 2D circles detected in the image.
   */
  std::vector<vpImageCircle> detect(const vpImage<vpRGBa> &I);

  /**
   * \brief Perform Circle Hough Transform to detect the circles in a gray-scale image
   *
   * \param[in] I The input gray scale image.
   * \return std::vector<vpImageCircle> The list of 2D circles detected in the image.
   */
  std::vector<vpImageCircle> detect(const vpImage<unsigned char> &I);

  /**
   * \brief Perform Circle Hough Transform to detect the circles in in a gray-scale image.
   * Get only the \b nbCircles circles having the greatest number of votes.
   *
   * \param[in] I The input gray scale image.
   * \param[in] nbCircles The number of circles we want to get. If negative, all the circles will be
   * returned, sorted such as result[0] has the highest number of votes and result[end -1] the lowest.
   * \return std::vector<vpImageCircle> The list of 2D circles with the most number
   * of votes detected in the image.
   */
  std::vector<vpImageCircle> detect(const vpImage<unsigned char> &I, const int &nbCircles);
  //@}

  /** @name  Configuration from files */
  //@{
#ifdef VISP_HAVE_NLOHMANN_JSON
  /**
   * \brief Construct a new vpCircleHoughTransform object configured according to
   * the JSON file whose path is \b jsonPath. Throw a \b vpException error if the file
   * does not exist.
   * \param[in] jsonPath The path towards the JSON configuration file.
   */
  vpCircleHoughTransform(const std::string &jsonPath);

  /**
   * \brief Initialize all the algorithm parameters using the JSON file
   * whose path is \b jsonPath. Throw a \b vpException error if the file
   * does not exist.
   *
   * \param[in] jsonPath The path towards the JSON configuration file.
   */
  void initFromJSON(const std::string &jsonPath);

  /**
   * \brief Save the configuration of the detector in a JSON file
   * described by the path \b jsonPath. Throw a \b vpException
   * is the file cannot be created.
   *
   * \param[in] jsonPath The path towards the JSON output file.
   */
  void saveConfigurationInJSON(const std::string &jsonPath) const;

  /**
   * \brief Read the detector configuration from JSON. All values are optional and if an argument is not present,
   * the default value defined in the constructor is kept
   *
   * \param[in] j The JSON object, resulting from the parsing of a JSON file.
   * \param[out] detector The detector, that will be initialized from the JSON data.
   */
  inline friend void from_json(const json &j, vpCircleHoughTransform &detector)
  {
    detector.m_algoParams = j;
  }

  /**
   * \brief Parse a vpCircleHoughTransform into JSON format.
   *
   * \param[out] j A JSON parser object.
   * \param[in] detector The vpCircleHoughTransform that must be parsed into JSON format.
   */
  inline friend void to_json(json &j, const vpCircleHoughTransform &detector)
  {
    j = detector.m_algoParams;
  }
#endif
  //@}

  /** @name  Setters */
  //@{
  /**
   * \brief Initialize all the algorithm parameters.
   *
   * \param[in] algoParams The algorithm parameters.
   */
  void init(const vpCircleHoughTransformParameters &algoParams);

  /**
   * \brief Permits to choose the filtering + gradient operators to use.
   *
   * \param[in] type The type of filtering + gradient operators to use.
   */
  inline void setFilteringAndGradientType(const vpImageFilter::vpCannyFilteringAndGradientType &type)
  {
    m_algoParams.m_filteringAndGradientType = type;
    m_cannyVisp.setFilteringAndGradientType(type);
    initGradientFilters();
  }

  /**
   * \brief Set the parameters of the Gaussian filter, that permits to blur the
   * gradients of the image.
   *
   * \param[in] kernelSize The size of the Gaussian kernel. Must be an odd value.
   * \param[in] stdev The standard deviation of the Gaussian function.
   */
  inline void setGaussianParameters(const int &kernelSize, const float &stdev)
  {
    m_algoParams.m_gaussianKernelSize = kernelSize;
    m_algoParams.m_gaussianStdev = stdev;

    if ((m_algoParams.m_gaussianKernelSize % 2) != 1) {
      throw vpException(vpException::badValue, "Gaussian Kernel size should be odd.");
    }

    if (m_algoParams.m_gaussianStdev <= 0) {
      throw vpException(vpException::badValue, "Standard deviation should be > 0");
    }

    initGaussianFilters();
  }

  /**
   * \brief Set the parameters of the gradient filter (Sobel or Scharr) kernel size filters.
   *
   * \param[in] apertureSize The size of the gradient filters kernel. Must be an odd value.
   */
  inline void setGradientFilterAperture(const unsigned int &apertureSize)
  {
    m_algoParams.m_gradientFilterKernelSize = apertureSize;

    if ((m_algoParams.m_gradientFilterKernelSize % 2) != 1) {
      throw vpException(vpException::badValue, "Gradient filter (Sobel or Scharr) Kernel size should be odd.");
    }

    initGradientFilters();
  }

  /**
   * \brief Set the backend to use to perform the Canny edge detection.
   *
   * \param[in] type The backend that must be used.
   */
  inline void setCannyBackend(const vpImageFilter::vpCannyBackendType &type)
  {
    m_algoParams.m_cannyBackendType = type;
  }

  /*!
   * Set the threshold for the Canny operator.
   * Only value greater than this value are marked as an edge.
   * If negative, the threshold is automatically computed.
   * \param[in] lowerCannyThreshold : Canny filter lower threshold. When set to -1 (default), compute
   * automatically this threshold.
   * \param[in] upperCannyThreshold : Canny filter upper threshold. When set to -1 (default), compute
   * automatically this threshold.
   */
  inline void setCannyThreshold(const float &lowerCannyThreshold, const float &upperCannyThreshold)
  {
    m_algoParams.m_lowerCannyThresh = lowerCannyThreshold;
    m_algoParams.m_upperCannyThresh = upperCannyThreshold;
  }

  /**
   * \brief Set the Canny thresholds ratio that are used to automatically compute the Canny thresholds
   * in case the user asks to.
   *
   * \sa \ref vpCircleHoughTransform::setCannyThreshold "vpCircleHoughTransform::setCannyThreshold(const float&, const float&)"
   *
   * \param[in] lowerThreshRatio The ratio of the upper threshold the lower threshold will be equal to.
   * \param[in] upperThreshRatio The ratio of pixels that must have a gradient lower than the upper threshold.
   */
  inline void setCannyThresholdRatio(const float &lowerThreshRatio, const float &upperThreshRatio)
  {
    m_algoParams.m_lowerCannyThreshRatio = lowerThreshRatio;
    m_algoParams.m_upperCannyThreshRatio = upperThreshRatio;
    m_cannyVisp.setCannyThresholdsRatio(lowerThreshRatio, upperThreshRatio);
  }

  /*!
   * Set circles center min distance.
   * Change this value to detect circles with different distances to each other.
   *
   * \param[in] center_min_dist : Center min distance in pixels.
   */
  inline void setCircleCenterMinDist(const float &center_min_dist)
  {
    m_algoParams.m_centerMinDist = center_min_dist;

    if (m_algoParams.m_centerMinDist <= 0) {
      throw vpException(vpException::badValue, "Circles center min distance  must be positive.");
    }
  }

  /*!
   * Set circles center min and max location in the image.
   * If one value is equal to \b std::numeric_limits<int>::min or
   * \b std::numeric_limits<int>::max(), the algorithm will set it
   * either to -maxRadius or +maxRadius depending on if
   * it is the lower or upper limit that is missing.
   *
   * \param[in] center_min_x : Center min location on the horizontal axis, expressed in pixels.
   * \param[in] center_max_x : Center max location on the horizontal axis, expressed in pixels.
   * \param[in] center_min_y : Center min location on the vertical axis, expressed in pixels.
   * \param[in] center_max_y : Center max location on the vertical axis, expressed in pixels.
   */
  void setCircleCenterBoundingBox(const int &center_min_x, const int &center_max_x,
                                          const int &center_min_y, const int &center_max_y)
  {
    m_algoParams.m_centerXlimits.first = center_min_x;
    m_algoParams.m_centerXlimits.second = center_max_x;
    m_algoParams.m_centerYlimits.first = center_min_y;
    m_algoParams.m_centerYlimits.second = center_max_y;
  }

  /*!
   * Set circles min radius.
   * \param[in] circle_min_radius : Min radius in pixels.
   */
  inline void setCircleMinRadius(const float &circle_min_radius)
  {
    m_algoParams.m_minRadius = static_cast<unsigned int>(circle_min_radius);
  }

  /*!
   * Set circles max radius.
   * \param[in] circle_max_radius : Max radius in pixels.
   */
  inline void setCircleMaxRadius(const float &circle_max_radius)
  {
    m_algoParams.m_maxRadius = static_cast<unsigned int>(circle_max_radius);
  }

  /*!
   * Set circles perfectness. The scalar product radius RC_ij . gradient(Ep_j) >=  m_circlePerfectness * || RC_ij || * || gradient(Ep_j) || to add a vote for the radius RC_ij.
   * \param[in] circle_perfectness : Circle perfectness. Value between 0 and 1. A perfect circle has value 1.
   */
  void setCirclePerfectness(const float &circle_perfectness)
  {
    m_algoParams.m_circlePerfectness = circle_perfectness;
    if (m_algoParams.m_circlePerfectness <= 0 || m_algoParams.m_circlePerfectness > 1) {
      throw vpException(vpException::badValue, "Circle perfectness must be in the interval ] 0; 1].");
    }
  }

  /**
   * \brief Set the parameters of the computation of the circle center candidates.
   *
   * \param[in] dilatationRepet Number of repetition of the dilatation operation to detect the maxima in the center accumulator.
   * \param[in] centerThresh Minimum number of votes a point must exceed to be considered as center candidate.
   */
  inline void setCenterComputationParameters(const int &dilatationRepet, const float &centerThresh)
  {
    m_algoParams.m_dilatationNbIter = dilatationRepet;
    m_algoParams.m_centerThresh = centerThresh;

    if (m_algoParams.m_centerThresh <= 0) {
      throw vpException(vpException::badValue, "Votes thresholds for center detection must be positive.");
    }
  }

  /**
   * \brief Set the parameters of the computation of the circle radius candidates.
   *
   * \param[in] radiusRatioThresh Minimum number of votes per radian a radius candidate RC_ij of a center candidate CeC_i must have in order that the circle of center CeC_i and radius RC_ij must be considered as circle candidate.
   */
  inline void setRadiusRatioThreshold(const float &radiusRatioThresh)
  {
    m_algoParams.m_circleProbaThresh = radiusRatioThresh;

    if (m_algoParams.m_circleProbaThresh <= 0) {
      throw vpException(vpException::badValue, "Radius ratio threshold must be > 0.");
    }
  }

  /**
   * \brief Set the radius merging threshold used during the merging step in order
   * to merge the circles that are similar.
   *
   * \param[in] radiusDifferenceThresh Maximum radius difference between two circle candidates to consider merging them.
   */
  inline void setRadiusMergingThresholds(const float &radiusDifferenceThresh)
  {
    m_algoParams.m_mergingRadiusDiffThresh = radiusDifferenceThresh;

    if (m_algoParams.m_mergingRadiusDiffThresh <= 0) {
      throw vpException(vpException::badValue, "Radius difference merging threshold must be positive.");
    }
  }
  //@}

  /** @name  Getters */
  //@{
  /**
   * \brief Get the list of Center Candidates, stored as pair <idRow, idCol>
   *
   * \return std::vector<std::pair<unsigned int, unsigned int> > The list of Center Candidates, stored as pair <idRow, idCol>
   */
  inline std::vector<std::pair<int, int> > getCenterCandidatesList()
  {
    return m_centerCandidatesList;
  }

  /**
   * \brief Get the number of votes of each Center Candidates.
   *
   * \return std::vector<int> The number of votes of each Center Candidates, ordered in the same way than \b m_centerCandidatesList.
   */
  inline std::vector<int> getCenterCandidatesVotes()
  {
    return m_centerVotes;
  }

  /**
   * \brief Get the Circle Candidates before merging step.
   *
   * \return std::vector<vpImageCircle> The list of circle candidates
   * that were obtained before the merging step.
   */
  inline std::vector<vpImageCircle> getCircleCandidates()
  {
    return m_circleCandidates;
  }

  /**
   * \brief Get the probabilities of the Circle Candidates.
   *
   * \return std::vector<float> The votes accumulator.
   */
  inline std::vector<float> getCircleCandidatesProbabilities()
  {
    return m_circleCandidatesProbabilities;
  }

  /**
   * \brief Get the gradient along the horizontal axis of the image.
   *
   * \return vpImage<float> The gradient along the horizontal axis of  the image.
   */
  inline vpImage<float> getGradientX()
  {
    return m_dIx;
  }

  /**
   * \brief Get the gradient along the vertical axis of the image.
   *
   * \return vpImage<float> The gradient along the vertical axis of  the image.
   */
  inline vpImage<float> getGradientY()
  {
    return m_dIy;
  }

  /**
   * \brief Get the Edge Map computed thanks to the Canny edge filter.
   *
   * \return vpImage<unsigned char> The edge map computed during the edge detection step.
   */
  inline vpImage<unsigned char> getEdgeMap()
  {
    return m_edgeMap;
  }

  /*!
   * Get internal Canny filter upper threshold. When value is equal to -1 (default), it means that the threshold is computed
   * automatically.
   */
  inline float getCannyThreshold() const
  {
    return m_algoParams.m_upperCannyThresh;
  }

  /*!
   * Get circles center min distance in pixels.
   */
  inline float getCircleCenterMinDist() const
  {
    return m_algoParams.m_centerMinDist;
  }

  /*!
   * Get circles min radius in pixels.
   */
  inline unsigned int getCircleMinRadius() const
  {
    return m_algoParams.m_minRadius;
  }

  /*!
   * Get circles max radius in pixels.
   */
  inline unsigned int getCircleMaxRadius() const
  {
    return m_algoParams.m_maxRadius;
  }

  /*!
   * Get the probabilities of the detections that are outputed by vpCircleHoughTransform::detect()
   */
  inline std::vector<float> getDetectionsProbabilities() const
  {
    return m_finalCirclesProbabilities;
  }

  /*!
   * Get the number of votes for the detections that are outputed by vpCircleHoughTransform::detect()
   */
  inline std::vector<unsigned int> getDetectionsVotes() const
  {
    return m_finalCircleVotes;
  }
  //@}

  /*!
   * Create a string with all Hough transform parameters.
   */
  std::string toString() const;

  /*!
   * Create a ostream with all Hough transform parameters.
   */
  friend VISP_EXPORT std::ostream &operator<<(std::ostream &os, const vpCircleHoughTransform &detector);

private:
  /**
   * \brief Initialize the Gaussian filters used to blur the image.
   */
  void initGaussianFilters();

  /**
   * \brief Initialize the gradient filters used to compute the gradient images.
   */
  void initGradientFilters();

  /**
   * \brief Perform Gaussian smoothing on the input image to reduce the noise
   * that would perturbate the edge detection.
   * Then, compute the x-gradient and y-gradient of the input images.
   *
   * \param[in] I The input gray scale image.
   */
  void computeGradientsAfterGaussianSmoothing(const vpImage<unsigned char> &I);

  /**
   * \brief Perform edge detection based on the computed gradients.
   * Stores the edge points and the edge points connectivity.
   *
   * \param[in] I The input gray scale image.
   */
  void edgeDetection(const vpImage<unsigned char> &I);

  /**
   * \brief Filter the edge map in order to remove isolated edge points.
   */
  void filterEdgeMap();

  /**
   * \brief Determine the image points that are circle center candidates.
   * Increment the center accumulator based on the edge points and gradient information.
   * Perform thresholding to keep only the center candidates that exceed the threshold.
   */
  void computeCenterCandidates();

  /**
   * \brief Compute the probability of \b circle given the number of pixels voting for
   * it \b nbVotes.
   * The probability is defined as the ratio of \b nbVotes by the theoretical number of
   * pixel that should be visible in the image.
   *
   * \param[in] circle The circle for which we want to evaluate the probability.
   * \param[in] nbVotes The number of visible pixels of the given circle.
   * \return float The probability of the circle.
   */
  float computeCircleProbability(const vpImageCircle &circle, const unsigned int &nbVotes);

  /**
   * \brief For each center candidate CeC_i, do:
   * - For each edge point EP_j, compute the distance d_ij = distance(CeC_i; EP_j)
   * - Determine to which radius candidate bin RCB_k the distance d_ij belongs to
   * - Increment the radius candidate accumulator accum_rc[CeC_i][RCB_k]
   * - If accum_rc[CeC_i][RCB_k] > radius_count_thresh, add the circle candidate (CeC_i, RCB_k)
   *   to the list of circle candidates
   */
  void computeCircleCandidates();

  /**
   * \brief For each circle candidate CiC_i do:
   * - For each other circle candidate CiC_j do:
   * +- Compute the similarity between CiC_i and CiC_j
   * +- If the similarity exceeds a threshold, merge the circle candidates CiC_i and CiC_j and remove CiC_j of the list
   * - Add the circle candidate CiC_i to the final list of detected circles
   */
  void mergeCircleCandidates();


  vpCircleHoughTransformParameters m_algoParams; /*!< Attributes containing all the algorithm parameters.*/
  // // Gaussian smoothing attributes
  vpArray2D<float> m_fg;

  // // Gradient computation attributes
  vpArray2D<float> m_gradientFilterX; /*!< Contains the coefficients of the gradient kernel along the X-axis*/
  vpArray2D<float> m_gradientFilterY; /*!< Contains the coefficients of the gradient kernel along the Y-axis*/
  vpImage<float> m_dIx; /*!< Gradient along the x-axis of the input image.*/
  vpImage<float> m_dIy; /*!< Gradient along the y-axis of the input image.*/

  // // Edge detection attributes
  vpCannyEdgeDetection m_cannyVisp; /*!< Edge detector ViSP implementation, used if ViSP has not been compiled with OpenCV imgproc module*/
  vpImage<unsigned char> m_edgeMap; /*!< Edge map resulting from the edge detection algorithm.*/

  // // Center candidates computation attributes
  std::vector<std::pair<unsigned int, unsigned int> > m_edgePointsList;       /*!< Vector that contains the list of edge points, to make faster some parts of the algo. They are stored as pair<#row, #col>.*/
  std::vector<std::pair<int, int> > m_centerCandidatesList; /*!< Vector that contains the list of center candidates. They are stored as pair<#row, #col>.*/
  std::vector<int> m_centerVotes; /*!< Number of votes for the center candidates that are kept.*/

  // // Circle candidates computation attributes
  std::vector<vpImageCircle> m_circleCandidates;        /*!< List of the candidate circles.*/
  std::vector<float> m_circleCandidatesProbabilities; /*!< Probabilities of each candidate circle that is kept.*/
  std::vector<unsigned int> m_circleCandidatesVotes; /*!< Number of pixels voting for each candidate circle that is kept.*/

  // // Circle candidates merging attributes
  std::vector<vpImageCircle> m_finalCircles; /*!< List of the final circles, i.e. the ones resulting from the merge of the circle candidates.*/
  std::vector<float> m_finalCirclesProbabilities; /*!< Probabilities of each final circle, i.e. resulting from the merge of the circle candidates.*/
  std::vector<unsigned int> m_finalCircleVotes; /*!< Number of votes for the final circles.*/
};
#endif
