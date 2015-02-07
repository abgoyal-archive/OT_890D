

#ifndef PLUGIN_DEBUG_ANDROID_H__
#define PLUGIN_DEBUG_ANDROID_H__

#ifdef ANDROID_PLUGINS

// Define PLUGIN_DEBUG_LOCAL in an individual C++ file to enable for
// that file only.

// Define PLUGIN_DEBUG_GLOBAL to 1 to turn plug-in debug for all
// Android plug-in code in this direectory.
#define PLUGIN_DEBUG_GLOBAL     0

#if PLUGIN_DEBUG_GLOBAL || defined(PLUGIN_DEBUG_LOCAL)
# define PLUGIN_LOG(A, B...)    do { LOGI( A , ## B ); } while(0)
#else
# define PLUGIN_LOG(A, B...)    do { } while(0)
#endif

#endif

#endif // defined(PLUGIN_DEBUG_ANDROID_H__)
