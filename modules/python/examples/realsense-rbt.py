#############################################################################
#
# ViSP, open source Visual Servoing Platform software.
# Copyright (C) 2005 - 2023 by Inria. All rights reserved.
#
# This software is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# See the file LICENSE.txt at the root directory of this source
# distribution for additional information about the GNU GPL.
#
# For using ViSP with software that can not be combined with the GNU
# GPL, please contact Inria about acquiring a ViSP Professional
# Edition License.
#
# See https://visp.inria.fr for more information.
#
# This software was developed at:
# Inria Rennes - Bretagne Atlantique
# Campus Universitaire de Beaulieu
# 35042 Rennes Cedex
# France
#
# If you have questions regarding the use of this file, please contact
# Inria at visp@inria.fr
#
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Description:
# ViSP Python bindings example
#
#############################################################################

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple
import numpy as np
import time
import faulthandler
faulthandler.enable()


from visp.core import CameraParameters, HomogeneousMatrix
from visp.core import Color, Display, ImageConvert
from visp.core import ImageGray, ImageUInt16, ImageRGBa, ImageFloat
from visp.io import ImageIo
from visp.rbt import RBTracker
from visp.display_utils import get_display
import pyrealsense2 as rs


try:
  import cv2
except:
  print('Could not import opencv-python! make sure that it is installed as it is required')
  import sys
  sys.exit(1)

import matplotlib.pyplot as plt




@dataclass
class FrameData:
  I: ImageGray
  IRGB: ImageRGBa
  I_depth: Optional[ImageFloat]


def read_data(depth_scale: Optional[float], IRGB: ImageRGBa, I: ImageGray, pipe: rs.pipeline):
  use_depth = depth_scale is not None
  iteration = 1
  while True:
    frames = pipe.wait_for_frames()
    I_np = np.asanyarray(frames.get_color_frame().as_frame().get_data())
    I_np = np.concatenate((I_np, np.ones_like(I_np[..., 0:1], dtype=np.uint8)), axis=-1)
    IRGB.resize(I_np.shape[0], I_np.shape[1])
    I_rgba_ref = IRGB.numpy()
    I_rgba_ref[...] = I_np
    ImageConvert.convert(IRGB, I, 0)
    I_depth_float = None
    if use_depth:
      I_depth_raw = np.asanyarray(frames.get_depth_frame().as_frame().get_data())
      I_depth_float = I_depth_raw.astype(np.float32) * depth_scale
    iteration += 1
    yield FrameData(I, IRGB, ImageFloat(I_depth_float))


def cam_from_rs_profile(profile) -> Tuple[CameraParameters, int, int]:
  intr = profile.as_video_stream_profile().get_intrinsics() # Downcast to video_stream_profile and fetch intrinsics
  return CameraParameters(intr.fx, intr.fy, intr.ppx, intr.ppy), intr.height, intr.width

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('--tracker', type=str, required=True,
                      help='Path to the json file containing the tracker configuration.')


  args = parser.parse_args()
  tracker_path = args.tracker
  assert Path(tracker_path).exists(), 'Tracker file not found'


  # Initialize realsense2
  pipe = rs.pipeline()
  config = rs.config()
  config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 60)
  config.enable_stream(rs.stream.color, 640, 480, rs.format.rgb8, 60)

  cfg = pipe.start(config)
  depth_scale = cfg.get_device().first_depth_sensor().get_depth_scale()


  tracker = RBTracker()

  tracker.loadConfigurationFile(tracker_path)


  cam_color, color_height, color_width = cam_from_rs_profile(cfg.get_stream(rs.stream.color))
  cam_depth, depth_height, depth_width = cam_from_rs_profile(cfg.get_stream(rs.stream.depth))


  # Camera intrinsics

  print('Color intrinsics:', cam_color)
  print('Depth intrinsics:', cam_depth)
  I = ImageGray()
  IRGB = ImageRGBa()
  I_depth_display = ImageGray()
  data_generator = read_data(depth_scale, IRGB, I, pipe)
  frame_data = next(data_generator) # Get first frame for init

  # Initialize displays
  dI = get_display()
  dI.init(I, 0, 0, 'Color image')

  I_depth = ImageGray()
  dDepth = get_display()

  ImageConvert.createDepthHistogram(frame_data.I_depth, I_depth)
  dDepth.init(I_depth,  I.getWidth(), 0, 'Depth')

  for frame in data_generator:
    Display.display(I)
    Display.displayText(I, 50, 0, 'Click to initialize tracking', Color.red)
    Display.flush(I)
    event = Display.getClick(I, blocking=False)
    if event:
      break

  tracker.initClick(I, str(mbt_model.init_file))
  start_time =  time.time()
  for frame_data in data_generator:
    if frame_data.I_depth is not None:
      ImageConvert.createDepthHistogram(frame_data.I_depth, I_depth)

    Display.display(I)
    if not args.disable_depth:
      Display.display(I_depth)

    if args.disable_depth:
      tracker.track(I=I)
    else:
      pc = frame_data.point_cloud
      image_dict = {
        'Camera1': I
      }
      t = time.time()
      tracker.track(image_dict, {'Camera2': pc.reshape(depth_height, depth_width, 3)})
    cMo = HomogeneousMatrix()
    tracker.getPose(cMo)

    Display.displayFrame(I, cMo, cam_color, 0.05, Color.none, 2)
    tracker.display(I, cMo, cam_color, Color.red, 2)
    Display.flush(I)
    if not args.disable_depth:
      Display.flush(I_depth)

    if args.step_by_step:
      Display.getKeyboardEvent(I, blocking=True)
    else:
      event = Display.getClick(I, blocking=False)
      if event:
        break
  end_time = time.time()
  print(f'total time = {end_time - start_time}s')