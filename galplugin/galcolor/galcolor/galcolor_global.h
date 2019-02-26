#ifndef GALCOLOR_GLOBAL_H
#define GALCOLOR_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(GALCOLOR_LIBRARY)
#  define GALCOLORSHARED_EXPORT Q_DECL_EXPORT
#else
#  define GALCOLORSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // GALCOLOR_GLOBAL_H