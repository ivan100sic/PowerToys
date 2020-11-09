// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Graphics.Capture;

namespace VideoGifCapture.Helpers
{
    class CaptureToy
    {
        public static void Run()
        {
            if (GraphicsCaptureSession.IsSupported())
            {
                throw new Exception("ok");
            }
        }
    }
}
