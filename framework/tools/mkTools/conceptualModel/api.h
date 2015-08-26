//--------------------------------------------------------------------------------------------------
/**
 * @file api.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD


struct Component_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a component's reference to an API file.
 */
//--------------------------------------------------------------------------------------------------
struct ApiFile_t
{
    std::string path;   ///< Absolute path to the .api file.

    std::string defaultPrefix; ///< Default prefix for generated code identifiers and files.

    std::list<ApiFile_t*> includes; ///< List of other .api files that this one uses types from.

    bool isIncluded;    ///< true if this .api file is included by other .api files (via USETYPES).

    std::string codeGenDir; ///< Path to code generation dir relative to working directory.

    // Functions to fetch relative paths to files (relative to root of working dir tree).
    std::string GetClientInterfaceFile (const std::string& internalName) const;
    std::string GetServerInterfaceFile(const std::string& internalName) const;
    std::string GetAsyncServerInterfaceFile(const std::string& internalName) const;

    /// Get a pre-existing API file object for the .api file at a given path.
    /// @return Pointer to the object or NULL if not found.
    static ApiFile_t* GetApiFile(const std::string& path);

    /// Create a new API file object for the .api file at a given path.
    /// @return Pointer to the object.
    /// @throw model::Exception_t if already exists.
    static ApiFile_t* CreateApiFile(const std::string& path);

    // Get a reference to the master map containing all the API files that have been referenced.
    static const std::map<std::string, ApiFile_t*>& GetApiFileMap() { return ApiFileMap; }

    // Get paths for all client-side interface .h files generated for all
    // .api files included by this one.  Results are added to the set provided.
    void GetClientUsetypesApiHeaders(std::set<std::string>& results);

    // Get paths for all server-side interface .h files generated for all
    // .api files included by this one.  Results are added to the set provided.
    void GetServerUsetypesApiHeaders(std::set<std::string>& results);

protected:

    ApiFile_t(const std::string& p);

    /// Map of file paths to pointers to API File objects.
    /// This is used to keep a single, unique API File object for each unique .api file.
    /// The key is the cannonical path to the .api file.  The value is a pointer to an object.
    static std::map<std::string, ApiFile_t*> ApiFileMap;

};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a component's reference to an API file.
 */
//--------------------------------------------------------------------------------------------------
struct ApiRef_t
{
    ApiFile_t* apiFilePtr;    ///< Pointer to the API file object.

    Component_t* componentPtr;  ///< Pointer to the component.

    const std::string internalName;   ///< Name used inside the component to refer to the interface.

    std::string interfaceFile;  ///< .h file that gets included by interfaces.h.

protected:

    ApiRef_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents inclusion of types from an IPC API interface definition (.api file).
 */
//--------------------------------------------------------------------------------------------------
struct ApiTypesOnlyInterface_t : public ApiRef_t
{
    ApiTypesOnlyInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a client-side IPC API interface.
 */
//--------------------------------------------------------------------------------------------------
struct ApiClientInterface_t : public ApiRef_t
{
    std::string internalHFile;  ///< local.h file that gets included by generated .c code.
    std::string sourceFile;     ///< Generated .c file.
    std::string objectFile;     ///< Path to the .o file for this interface.

    bool manualStart;   ///< true = generated main() should not call the ConnectService() function.

    ApiClientInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a server-side IPC API interface.
 */
//--------------------------------------------------------------------------------------------------
struct ApiServerInterface_t : public ApiRef_t
{
    std::string internalHFile;  ///< local.h file that gets included by generated .c code.
    std::string sourceFile;     ///< Generated .c file.
    std::string objectFile;     ///< Path to the .o file for this interface.

    const bool async;         ///< true = component wants to use asynchronous mode of operation.
    bool manualStart;   ///< true = generated main() should not call AdvertiseService() function.

    ApiServerInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName, bool async);
};


struct ComponentInstance_t;
struct Binding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of an IPC API interface within an executable.
 *
 * This is a base class that cannot be instantiated on its own.
 */
//--------------------------------------------------------------------------------------------------
struct ApiInterfaceInstance_t
{
    /// Component instance this interface instance belongs to.
    ComponentInstance_t* componentInstancePtr;

    std::string name;   ///< Name used to identify this interface to the service directory.

    bool isExternal;    ///< true if the interface is one of the app's external interfaces.

protected:

    ApiInterfaceInstance_t(ComponentInstance_t* cInstPtr, const std::string& internalName);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of a client-side IPC API interface within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ApiClientInterfaceInstance_t : public ApiInterfaceInstance_t
{
    ApiClientInterface_t* ifPtr;    ///< Interface this is an instance of.

    Binding_t* bindingPtr;  ///< Ptr to the binding, or NULL if not bound.

    ApiClientInterfaceInstance_t(ComponentInstance_t* cInstPtr, ApiClientInterface_t* p);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of a server-side IPC API interface within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ApiServerInterfaceInstance_t : public ApiInterfaceInstance_t
{
    ApiServerInterface_t* ifPtr;    ///< Interface this is an instance of.

    ApiServerInterfaceInstance_t(ComponentInstance_t* cInstPtr, ApiServerInterface_t* p);
};



#endif // LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD
