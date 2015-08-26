//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkcomp" functionality of the "mk" tool.
 *
 *  Run 'mkcomp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include <string.h>
#include <unistd.h>

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{



/// Object that stores build parameters that we gather.
static mk::BuildParams_t BuildParams;

/// Path to the directory containing the component to be built.
std::string ComponentPath;

/// Full path of the library file to be generated. "" = use default file name.
std::string BuildOutputPath = "";

/// true if interface instance libraries should be built and linked with the component library
/// so that the component library can be linked to and used in more traditional ways, without
/// the use of mkexe or mkapp.
static bool IsStandAlone = false;

// true if the build.ninja file should be ignored and everything should be regenerated, including
// a new build.ninja.
static bool DontRunNinja = false;


//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments and update the static operating parameters variables.
 *
 * Throws a std::runtime_error exception on failure.
 **/
//--------------------------------------------------------------------------------------------------
static void GetCommandLineArgs
(
    int argc,
    const char** argv
)
//--------------------------------------------------------------------------------------------------
{
    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [](const char* arg)
    {
        BuildParams.cFlags += " ";
        BuildParams.cFlags += arg;
    };

    // Lambda function that gets called for each occurence of the --cxxflags, (or -X) argument on
    // the command line.
    auto cxxFlagsPush = [](const char* arg)
    {
        BuildParams.cxxFlags += " ";
        BuildParams.cxxFlags += arg;
    };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [](const char* arg)
    {
        BuildParams.ldFlags += " ";
        BuildParams.ldFlags += arg;
    };

    // Lambda functions for handling arguments that can appear more than once on the
    // command line.
    auto interfaceDirPush = [](const char* path)
    {
        BuildParams.interfaceDirs.push_back(path);
    };
    auto sourceDirPush = [](const char* path)
    {
        BuildParams.sourceDirs.push_back(path);
    };

    // Lambda function that gets called once for each occurence of a component path on the
    // command line.
    auto componentPathSet = [](const char* param)
    {
        static bool matched = false;
        if (matched)
        {
            throw mk::Exception_t("Only one component allowed. First is '" + ComponentPath
                              + "'.  Second is '" + param + "'.");
        }
        matched = true;

        ComponentPath = param;
    };

    // Register all our arguments with the argument parser.
    args::AddOptionalString(&BuildOutputPath,
                             "",
                             'o',
                             "output-path",
                             "Specify the complete path name of the component library to be built.");

    args::AddOptionalString(&BuildParams.libOutputDir,
                             ".",
                             'l',
                             "lib-output-dir",
                             "Specify the directory into which any generated runtime libraries"
                             " should be put.  (This option ignored if -o specified.)");

    args::AddOptionalString(&BuildParams.workingDir,
                             "_build",
                             'w',
                             "object-dir",
                             "Specify the directory into which any intermediate build artifacts"
                             " (such as .o files and generated source code files) should be put.");

    args::AddOptionalString(&BuildParams.target,
                             "localhost",
                             't',
                             "target",
                             "Specify the target device to build for (e.g., localhost or ar7).");

    args::AddMultipleString('i',
                             "interface-search",
                             "Add a directory to the interface search path.",
                             interfaceDirPush);

    args::AddMultipleString('c',
                             "component-search",
                             "(DEPRECATED) Add a directory to the source search path (same as -s).",
                             sourceDirPush);

    args::AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path.",
                             sourceDirPush);

    args::AddOptionalFlag(&BuildParams.beVerbose,
                           'v',
                           "verbose",
                           "Set into verbose mode for extra diagnostic information.");

    args::AddMultipleString('C',
                             "cflags",
                             "Specify extra flags to be passed to the C compiler.",
                             cFlagsPush);

    args::AddMultipleString('X',
                             "cxxflags",
                             "Specify extra flags to be passed to the C++ compiler.",
                             cxxFlagsPush);

    args::AddMultipleString('L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables.",
                             ldFlagsPush);

    args::AddOptionalFlag(&IsStandAlone,
                           'a',
                           "stand-alone",
                           "Build the component library and all its sub-components' libraries"
                           " such that the component library can be loaded and run"
                           " without the help of mkexe or mkapp.  This is useful when integrating"
                           " with third-party code that is built using some other build system." );

    args::AddOptionalFlag(&DontRunNinja,
                           'n',
                           "dont-run-ninja",
                           "Even if a build.ninja file exists, ignore it, parse all inputs, and"
                           " generate all output files, including a new copy of the build.ninja,"
                           " then exit without running ninja.  This is used by the build.ninja to"
                           " to regenerate itself and any other files that need to be regenerated"
                           " when the build.ninja finds itself out of date.");

    args::AddOptionalFlag(&BuildParams.codeGenOnly,
                          'g',
                          "generate-code",
                          "Only generate code, but don't compile or link anything."
                          " The interface definition (include) files will be generated, along"
                          " with component main files."
                          " This is useful for supporting context-sensitive auto-complete and"
                          " related features in source code editors, for example.");

    // Any remaining parameters on the command-line are treated as a component path.
    // Note: there should only be one.
    args::SetLooseArgHandler(componentPathSet);

    // Scan the arguments now.
    args::Scan(argc, argv);

    // Were we given a component?
    if (ComponentPath == "")
    {
        throw std::runtime_error("A component must be supplied on the command line.");
    }

    // Add the current working directory to the list of source search directories and the
    // list of interface search directories.
    BuildParams.sourceDirs.push_back(".");
    BuildParams.interfaceDirs.push_back(".");

    // Add the Legato framework's "interfaces" directory to the list of interface search dirs.
    BuildParams.interfaceDirs.push_back(path::Combine(envVars::Get("LEGATO_ROOT"), "interfaces"));
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkcomp functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeComponent
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    envVars::SetTargetSpecific(BuildParams.target);

    // If we have not been asked to ignore any already existing build.ninja, and the command-line
    // arguments and environment variables we were given are the same as last time, just run ninja.
    if (!DontRunNinja)
    {
        if (args::MatchesSaved(BuildParams, argc, argv) && envVars::MatchesSaved(BuildParams))
        {
            RunNinja(BuildParams);
            // NOTE: If build.ninja exists, RunNinja() will not return.  If it doesn't it will.
        }
    }

    // Locate the component.
    auto foundPath = file::FindComponent(ComponentPath, BuildParams.sourceDirs);
    if (foundPath == "")
    {
        throw mk::Exception_t("Couldn't find component '" + ComponentPath + "'.");
    }
    ComponentPath = path::MakeAbsolute(foundPath);

    // Generate the conceptual object model.
    model::Component_t* componentPtr = modeller::GetComponent(ComponentPath, BuildParams);
    if (!BuildOutputPath.empty())
    {
        componentPtr->lib = BuildOutputPath;
    }

    // Generate a custom "interfaces.h" file for this component.
    code::GenerateInterfacesHeader(componentPtr, BuildParams);

    // Generate a custom "_componentMain.c" file for this component.
    code::GenerateComponentMainFile(componentPtr, BuildParams, IsStandAlone);

    // Generate the ninja build script.
    ninja::Generate(componentPtr, BuildParams, argc, argv);

    // If we haven't been asked not to run ninja,
    if (!DontRunNinja)
    {
        // Save the command-line arguments and environment variables for future comparison.
        // Note: we don't need to do this if we have been asked not to run ninja, because
        // that only happens when ninja is already running and asking us to regenerate its
        // script for us, and that only happens if we just saved the args and env vars and
        // ran ninja.
        args::Save(BuildParams, argc, argv);
        envVars::Save(BuildParams);

        RunNinja(BuildParams);
    }
}


} // namespace cli
