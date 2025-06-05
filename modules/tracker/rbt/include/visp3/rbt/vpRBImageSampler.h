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
 */

/*!
  \file vpRBImageSampler.h
  \brief Image sampling utils
*/
#ifndef VP_RB_IMAGE_SAMPLER_H
#define VP_RB_IMAGE_SAMPLER_H

#include <visp3/core/vpConfig.h>
#if defined(VISP_HAVE_NLOHMANN_JSON)
#include VISP_NLOHMANN_JSON(json.hpp)
#endif

class vpRBFeatureTrackerInput;

BEGIN_VISP_NAMESPACE

class VISP_EXPORT vpRBImageSampler
{
public:
  virtual ~vpRBImageSampler() = default;
  virtual std::pair<unsigned int, unsigned int> getSampleSteps(const vpRBFeatureTrackerInput &) = 0;

#if defined(VISP_HAVE_NLOHMANN_JSON)
  static std::shared_ptr<vpRBImageSampler> loadSampler(const nlohmann::json &);
#endif

};

class VISP_EXPORT vpRBFixedStepImageSampler : public vpRBImageSampler
{
public:
  vpRBFixedStepImageSampler(unsigned int step);
  std::pair<unsigned int, unsigned int> getSampleSteps(const vpRBFeatureTrackerInput &) VP_OVERRIDE;

private:
  unsigned int m_step; // Step in pixels
};

class VISP_EXPORT vpRBBoundingBoxImageSampler : public vpRBImageSampler
{
public:
  vpRBBoundingBoxImageSampler(double coverage);
  std::pair<unsigned int, unsigned int> getSampleSteps(const vpRBFeatureTrackerInput &) VP_OVERRIDE;

private:
  double m_coverage; // Step in pixels
};

class VISP_EXPORT vpRBTargetImageSampler : public vpRBImageSampler
{
public:
  vpRBTargetImageSampler(unsigned int target);
  std::pair<unsigned int, unsigned int> getSampleSteps(const vpRBFeatureTrackerInput &) VP_OVERRIDE;

private:
  unsigned int m_target; // Step in pixels
};




END_VISP_NAMESPACE

#endif
