//--------------------------------------------------------------------------------------------------
/**
 * @file interfacesHeaderGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/interfaces.h";

    if (buildParams.beVerbose)
    {
        std::cout << "Generating interfaces.h for component '" << componentPtr->name << "'"
                     "in '" << filePath << "'." << std::endl;
    }

    // Make sure the working file output directory exists.
    file::MakeDir(outputDir);

    // Open the interfaces.h file for writing.
    std::ofstream fileStream(filePath, std::ofstream::trunc);
    if (!fileStream.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for writing.");
    }

    std::string includeGuardName = "__" + componentPtr->name
                                        + "_COMPONENT_INTERFACE_H_INCLUDE_GUARD";

    fileStream << "/*\n"
                  " * AUTO-GENERATED interface.h for the " << componentPtr->name << " component.\n"
                  "\n"
                  " * Don't bother hand-editing this file.\n"
                  " */\n"
                  "\n"
                  "#ifndef " << includeGuardName << "\n"
                  "#define " << includeGuardName << "\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n"
                  "\n";

    // #include the client-side .h for each of the .api files from which only data types are used.
    for (auto interfacePtr : componentPtr->typesOnlyApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_interface.h" << "\"\n";
    }

    // For each of the component's client-side interfaces, #include the client-side .h file.
    for (auto interfacePtr : componentPtr->clientApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_interface.h" << "\"\n";
    }

    // For each of the component's server-side interfaces, #include the server-side .h file.
    for (auto interfacePtr : componentPtr->serverApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_server.h" << "\"\n";
    }

    // Put the finishing touches on interfaces.h.
    fileStream << "\n"
                  "#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n"
                  "\n"
                  "#endif // " << includeGuardName << "\n";
}


} // namespace code
