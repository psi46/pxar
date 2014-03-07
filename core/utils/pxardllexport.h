#ifndef PXARDLLEXPORT
#define PXARDLLEXPORT

/** Export classes from the DLL under WIN32 (not when using rootcint) */
#if ((defined WIN32) && (!defined __CINT__))
#define DLLEXPORT __declspec( dllexport )
#else
#define DLLEXPORT
#endif

#endif
