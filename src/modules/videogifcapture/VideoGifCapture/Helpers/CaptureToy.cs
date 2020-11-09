// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Graphics;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.Graphics.DirectX.Direct3D11;
using Windows.UI.Xaml.Controls;

namespace VideoGifCapture.Helpers
{
    class CaptureToy
    {
        public static async void Run()
        {
            if (GraphicsCaptureSession.IsSupported())
            {
                var picker = new GraphicsCapturePicker();
                var item = await picker.PickSingleItemAsync();

                if (item == null)
                {
                    throw new Exception();
                }

                IDirect3DDevice canvasDevice = null;
                Direct3D11CaptureFramePool framePool;

                framePool = Direct3D11CaptureFramePool.Create(
                    canvasDevice,
                    DirectXPixelFormat.B8G8R8A8UIntNormalized,
                    2,
                    item.Size);


                GraphicsCaptureSession session = framePool.CreateCaptureSession(item);
                framePool.FrameArrived += FramePool_FrameArrived;
                session.StartCapture();
            }
        }

        private static void FramePool_FrameArrived(Direct3D11CaptureFramePool sender, object args)
        {
        }
    }
}
