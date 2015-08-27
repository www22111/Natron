/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
#define NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/version.hpp>
#endif
#include "Engine/TextureRect.h"

#define TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR 2
#define TEXTURE_RECT_VERSION TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR
namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          TextureRect &t,
          const unsigned int version)
{

    ar & t.x1 & t.x2 & t.y1 & t.y2 & t.w & t.h & t.closestPo2;
    if (version >= TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR) {
        ar & t.par;
    }
}
}
}

BOOST_CLASS_VERSION(TextureRect, TEXTURE_RECT_VERSION);

#endif // NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
