// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <iostream>
#include <ostream>
#include <type_traits>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "model.h"
#include "obj_loader.h"

//-------------------------------------------------------------------------------------------------
// Helpers
//

enum model_type { OBJ, Unknown };

static model_type get_type(std::string const& filename)
{
    std::unordered_map<std::string, model_type> ext2type;
    ext2type.insert({ ".obj", OBJ });
    ext2type.insert({ ".OBJ", OBJ });

    boost::filesystem::path p(filename);

    auto result = ext2type.find(p.extension().string());

    if (result != ext2type.end())
    {
        return result->second;
    }

    return Unknown;
}

// FN is either a single filename (std::string)
// or a list of filenames (std::vector<std::string>)
template <typename FN>
bool load_model(FN const& fn, visionaray::model& mod, model_type mt)
{
    static_assert(
            std::is_same<FN, std::string>::value ||
            std::is_same<FN, std::vector<std::string>>::value,
            "Type mismatch"
            );

    switch (mt)
    {
    case OBJ:
        load_obj(fn, mod);
        return true;

    default:
        return false;
    }

    return false;
}


namespace visionaray
{

model::model()
{
    bbox.invalidate();
}

bool model::load(std::string const& filename)
{
    std::vector<std::string> filenames(1);

    filenames[0] = filename;

    return load(filenames);
}

bool model::load(std::vector<std::string> const& filenames)
{
    if (filenames.size() < 1)
    {
        return false;
    }

    std::string fn(filenames[0]);
    std::replace(fn.begin(), fn.end(), '\\', '/');
    model_type mt = get_type(fn);

    bool same_model_type = true;

    for (size_t i = 1; i < filenames.size(); ++i)
    {
        std::string fni(filenames[i]);
        std::replace(fni.begin(), fni.end(), '\\', '/');
        model_type mti = get_type(fni);

        if (mti != mt)
        {
            same_model_type = false;
        }
    }

    try
    {
        if (same_model_type)
        {
            return load_model(filenames, *this, mt);
        }
        else
        {
            bool success = true;

            for (auto filename : filenames)
            {
                if (!load_model(filename, *this, get_type(filename)))
                {
                    success = false;
                    break;
                }
            }

            return success;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
    catch (...)
    {
        return false;
    }

    return false;
}

} // visionaray
