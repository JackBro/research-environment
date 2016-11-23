/// Copyright (c) 2015 Jonathan Moore
///
/// This software is provided 'as-is', without any express or implied warranty. 
/// In no event will the authors be held liable for any damages arising from 
/// the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, 
/// including commercial applications, and to alter it and redistribute it 
/// freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; 
/// you must not claim that you wrote the original software. 
/// If you use this software in a product, an acknowledgment in the product 
/// documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, 
/// and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.
namespace System.Extensions.Drawing
{
  using System;
  using System.Drawing;
  using System.Drawing.Imaging;
  using System.IO;

  /// <summary>
  /// Utility and extension class for <see cref="Image"/> objects.
  /// </summary>
  public static class ImageExtensions
  {
    /// <summary>
    /// Converts the specified byte array to an image.
    /// </summary>
    /// <param name="buffer">The byte array representation of the image.</param>
    /// <returns>The <see cref="Image"/> that was loaded from the byte array.</returns>
    public static Image FromByteArray(byte[] buffer)
    {
      if (buffer == null)
      {
        throw new ArgumentNullException(nameof(buffer));
      }

      using (MemoryStream memoryStream = new MemoryStream(buffer))
      {
        return Image.FromStream(memoryStream);
      }
    }

    /// <summary>
    /// Resizes an image, maintaining width:height ratios.
    /// </summary>
    /// <param name="image">The <see cref="Image"/> that you wish to resize.</param>
    /// <param name="width">The desired width of the resulting image.</param>
    /// <param name="height">The desired height of the resulting image.</param>
    /// <param name="pad">Whether the image should pad the borders to keep width/height.</param>
    /// <returns>The resulting resized <see cref="Image"/> object.</returns>
    public static Image Resize(this Image image, int width, int height, bool pad)
    {
      if (image == null)
      {
        throw new ArgumentNullException(nameof(image));
      }

      // calculate resize ratio
      double ratio = (double)width / (double)image.Width;
      if (ratio * image.Height > height)
      {
        ratio = (double)height / (double)image.Height;
      }

      var newHeight = (int)Math.Round(ratio * image.Height, 0);
      var newWidth = (int)Math.Round(ratio * image.Width, 0);

      if (!pad)
      {
        width = newWidth;
        height = newHeight;
      }

      var bmp = new Bitmap(width, height);

      // get the graphics object, set it up for quality resizing, and resize
      using (Graphics g = Graphics.FromImage(bmp))
      {
        g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
        g.DrawImage(image, (width - newWidth) / 2, (height - newHeight) / 2, newWidth, newHeight);
      }

      return bmp;
    }

    /// <summary>
    /// Converts the specified image to a byte array using format specified by the image.
    /// </summary>
    /// <param name="image">The <see cref="Image"/> to convert.</param>
    /// <returns>The byte array representation of the image.</returns>
    public static byte[] ToByteArray(this Image image)
    {
      if (image == null)
      {
        throw new ArgumentNullException(nameof(image));
      }

      return image.ToByteArray(image.RawFormat);
    }

    /// <summary>
    /// Converts the specified image to a byte array using the specified format.
    /// </summary>
    /// <param name="image">The <see cref="Image"/> to convert.</param>
    /// <param name="format">The <see cref="ImageFormat"/> to use when serializing.</param>
    /// <returns>The byte array representation of the image.</returns>
    public static byte[] ToByteArray(this Image image, ImageFormat format)
    {
      if (image == null)
      {
        throw new ArgumentNullException(nameof(image));
      }

      using (MemoryStream stream = new MemoryStream())
      {
        image.Save(stream, format);
        return stream.ToArray();
      }
    }

    /// <summary>
    /// Alpha blends an image with a background color;
    /// </summary>
    /// <param name="image">The <see cref="Image"/> to alpha blend.</param>
    /// <param name="alpha">The alpha value to use when blending.</param>
    /// <param name="backgroundColor">The background color to blend with.</param>
    /// <returns>A new <see cref="Image"/> with the alpha blended image.</returns>
    public static Image ToAlphaBlend(this Image image, int alpha, Color backgroundColor)
    {
      var bmp = new Bitmap(image.Width, image.Height);

      using (Graphics g = Graphics.FromImage(bmp))
      {
        g.DrawImage(image, 0, 0);
        using (SolidBrush backgroundBrush = new SolidBrush(Color.FromArgb(alpha, backgroundColor)))
        {
          g.FillRectangle(backgroundBrush, 0, 0, bmp.Width, bmp.Height);
        }
      }

      return bmp;
    }

    /// <summary>
    /// Grayscales an image.
    /// </summary>
    /// <param name="image">The <see cref="Image"/> that you wish to grayscale.</param>
    /// <returns>A new <see cref="Image"/> instance that represents the grayscaled image.</returns>
    public static Image ToGrayscale(this Image image)
    {
      var newImage = new Bitmap(image.Width, image.Height);
      using (Graphics g = Graphics.FromImage(newImage))
      {
        var colorMatrix = new ColorMatrix(new float[][]
                                                                {
                                                                 new float[] { 0.3f, 0.3f, 0.3f, 0, 0 },
                                                                 new float[] { 0.59f, 0.59f, 0.59f, 0, 0 },
                                                                 new float[] { 0.11f, 0.11f, 0.11f, 0, 0 },
                                                                 new float[] { 0, 0, 0, 1, 0, 0 },
                                                                 new float[] { 0, 0, 0, 0, 1, 0 },
                                                                 new float[] { 0, 0, 0, 0, 0, 1 }
                                                                });

        using (ImageAttributes imageAttr = new ImageAttributes())
        {
          imageAttr.SetColorMatrix(colorMatrix);
          g.DrawImage(image, new Rectangle(0, 0, image.Width, image.Height), 0, 0, image.Width, image.Height, GraphicsUnit.Pixel, imageAttr);
        }
      }

      return newImage;
    }
  }
}
