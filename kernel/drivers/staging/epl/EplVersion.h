

#ifndef _EPL_VERSION_H_
#define _EPL_VERSION_H_

// NOTE:
// All version macros should contain the same version number. But do not use
// defines instead of the numbers. Because the macro EPL_STRING_VERSION() can not
// convert a define to a string.
//
// Format: maj.min.build
//         maj            = major version
//             min        = minor version (will be set to 0 if major version will be incremented)
//                 build  = current build (will be set to 0 if minor version will be incremented)
//
#define DEFINED_STACK_VERSION       EPL_STACK_VERSION   (1, 3, 0)
#define DEFINED_OBJ1018_VERSION     EPL_OBJ1018_VERSION (1, 3, 0)
#define DEFINED_STRING_VERSION      EPL_STRING_VERSION  (1, 3, 0)

// -----------------------------------------------------------------------------
#define EPL_PRODUCT_NAME            "EPL V2"
#define EPL_PRODUCT_VERSION         DEFINED_STRING_VERSION
#define EPL_PRODUCT_MANUFACTURER    "SYS TEC electronic GmbH"

#define EPL_PRODUCT_KEY         "SO-1083"
#define EPL_PRODUCT_DESCRIPTION "openPOWERLINK Protocol Stack Source"

#endif // _EPL_VERSION_H_

// Die letzte Zeile muﬂ unbedingt eine leere Zeile sein, weil manche Compiler
// damit ein Problem haben, wenn das nicht so ist (z.B. GNU oder Borland C++ Builder).
