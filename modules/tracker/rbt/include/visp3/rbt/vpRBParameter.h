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
#ifndef VP_RB_PARAMETER_H
#define VP_RB_PARAMETR_H

#include <visp3/core/vpConfig.h>

#if defined(VISP_HAVE_NLOHMANN_JSON)
#include VISP_NLOHMANN_JSON(json.hpp)
#endif

class vpRBFeatureTrackerInput;

BEGIN_VISP_NAMESPACE

enum vpRBParameterUnit
{
  PIXELS = 0,
  METERS = 1
};

class VISP_EXPORT vpRBParameter
{
public:
  virtual ~vpRBParameter() = default;
  virtual double getValue(const vpRBFeatureTrackerInput &) = 0;

  vpRBParameterUnit getUnits() const { return m_units; }

#if defined(VISP_HAVE_NLOHMANN_JSON)
  static std::shared_ptr<vpRBParameter> loadParameter(const nlohmann::json &);
#endif

private:
  vpRBParameterUnit m_units;

};

class VISP_EXPORT vpRBFixedValueParameter : public vpRBParameter
{
public:
  vpRBFixedValueParameter(double value);
  double getValue(const vpRBFeatureTrackerInput &) VP_OVERRIDE;

private:
  double value; // Step in pixels
};

class VISP_EXPORT vpRBParameterRelativeToObjectSize : public vpRBParameter
{
public:
  vpRBParameterRelativeToObjectSize(double percentage);
  double getValue(const vpRBFeatureTrackerInput &) VP_OVERRIDE;

private:
  double m_coverage; // Step in pixels
};





END_VISP_NAMESPACE

#endif
