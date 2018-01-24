#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MUTATORS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MUTATORS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MUTATORS_EXPORTS
#define MUTATORS_API __declspec(dllexport)
#else
#define MUTATORS_API __declspec(dllimport)
#endif


//If you're combining all of the mutators into one project, uncomment this to give them all
//unique names
//#define ALL_MUTATORS_IN_ONE
#ifndef ALL_MUTATORS_IN_ONE
#define FUNCNAME(name) name
#else
#define FUNCNAME(name) MUTATOR_NAME ## _ ## name
#endif
