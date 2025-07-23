// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <algorithm>
#include <utility>

#include <boost/filesystem.hpp>

#include "image.h"
#include "png_image.h"

//-------------------------------------------------------------------------------------------------
// Helpers
//

enum image_type { PNG, Unknown };

static image_type get_type(std::string const& filename)
{
    boost::filesystem::path p(filename);

    // PNG

    static const std::string png_extensions[] = { ".png", ".PNG" };

    if (std::find(png_extensions, png_extensions + 2, p.extension()) != png_extensions + 2)
    {
        return PNG;
    }

    return Unknown;
}

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// image members
//

image::image(int width, int height, pixel_format format, uint8_t const* data)
    : image_base(width, height, format, data)
{
}

bool image::load(std::string const& filename)
{
    std::string fn(filename);
    std::replace(fn.begin(), fn.end(), '\\', '/');
    image_type it = get_type(fn);

    switch (it)
    {
    case PNG:
    {
        png_image png;
        if (png.load(fn))
        {
            width_  = png.width_;
            height_ = png.height_;
            format_ = png.format_;
            data_   = std::move(png.data_);
            return true;
        }
        return false;
    }

    // not supported

    case Unknown:
        // fall-through
    default:
        break;
    }

    return false;
}

bool image::save(std::string const& filename, file_base::save_options const& options)
{
    std::string fn(filename);
    std::replace(fn.begin(), fn.end(), '\\', '/');
    image_type it = get_type(fn);

    switch (it)
    {
    case PNG:
    {
        png_image png(width(), height(), format(), data());
        return png.save(fn, options);
    }

    // not supported

    case Unknown:
        // fall-through
    default:
        break;
    }

    return false;
}

} // visionaray
