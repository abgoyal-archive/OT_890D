

#ifndef QWEBKITGLOBAL_H
#define QWEBKITGLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QT_MAKEDLL)        /* create a Qt DLL library */
#  if defined(BUILD_WEBKIT)
#      define QWEBKIT_EXPORT Q_DECL_EXPORT
#  else
#      define QWEBKIT_EXPORT Q_DECL_IMPORT
#  endif
#elif defined(QT_DLL) /* use a Qt DLL library */
#  define QWEBKIT_EXPORT Q_DECL_IMPORT
#endif

#if !defined(QWEBKIT_EXPORT)
#  if defined(QT_SHARED)
#    define QWEBKIT_EXPORT Q_DECL_EXPORT
#  else
#    define QWEBKIT_EXPORT
#  endif
#endif

#if QT_VERSION < 0x040400
    #ifndef QT_BEGIN_NAMESPACE
    #define QT_BEGIN_NAMESPACE
    #endif

    #ifndef QT_END_NAMESPACE
    #define QT_END_NAMESPACE
    #endif
#endif


#endif // QWEBKITGLOBAL_H
