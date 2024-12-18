function(private_add_aax_wrapper_sources)
    set(oneValueArgs TARGET)
    cmake_parse_arguments(PAX "" "${oneValueArgs}" "" ${ARGN})

    set(tg ${PAX_TARGET})
    set(sd ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${tg} PUBLIC CLAP_WRAPPER_BUILD_FOR_AAX=1)

    if(WIN32)
    # this was the VST3 module init stuff - probably we need this in AAX at some point, too   -- aax?
    #    target_sources(${tg} PRIVATE ${sd}/src/detail/os/windows.cpp)
    elseif (APPLE)
    #    target_sources(${tg} PRIVATE ${sd}/src/detail/os/macos.mm)
    elseif(UNIX)
    #    target_sources(${tg} PRIVATE ${sd}/src/detail/os/linux.cpp)
    endif()

   
    target_sources(${tg} PRIVATE

           ${sd}/src/wrapasaax.h
           ${sd}/src/wrapasaax.cpp
 #          ${sd}/src/wrapasaax_entry.cpp
 #          # ${sd}/src/detail/ara/ara.h
 #          ${sd}/src/detail/aax/parameter.h
 #          ${sd}/src/detail/aax/parameter.cpp
 #          ${sd}/src/detail/aax/plugview.h
 #          ${sd}/src/detail/aax/plugview.cpp
 #          ${sd}/src/detail/aax/state.h
 #          ${sd}/src/detail/aax/process.h
 #          ${sd}/src/detail/aax/process.cpp
 #          ${sd}/src/detail/aax/categories.h
 #          ${sd}/src/detail/aax/categories.cpp
            # ${sd}/src/detail/aax/araaax.h - later
            )

    target_include_directories(${AX_TARGET}-clap-wrapper-aax-lib PRIVATE "${sd}/include")
endfunction(private_add_aax_wrapper_sources)

# define libraries
function(target_add_aax_wrapper)
    set(oneValueArgs
            TARGET
            OUTPUT_NAME
            #SUPPORTS_ALL_NOTE_EXPRESSIONS
            #SINGLE_PLUGIN_TUID

            BUNDLE_IDENTIFIER
            BUNDLE_VERSION

            MACOS_EMBEDDED_CLAP_LOCATION
            )
    cmake_parse_arguments(AX "" "${oneValueArgs}" "" ${ARGN} )

    message(INFO "target add aax wrapper")

    guarantee_aaxsdk()

    if (NOT DEFINED AX_TARGET)
        message(FATAL_ERROR "clap-wrapper: target_add_aax_wrapper requires a target")
    endif()

    if (NOT DEFINED AX_OUTPUT_NAME)
        message(FATAL_ERROR "clap-wrapper: target_add_aax_wrapper requires an output name")
    endif()

    message(STATUS "clap-wrapper: Adding AAX Wrapper to target ${AX_TARGET} generating '${AX_OUTPUT_NAME}.aaxplugin'")

    string(MAKE_C_IDENTIFIER ${AX_OUTPUT_NAME} outidentifier)

    #maybe needed later
    message(INFO AAX_SDK_ROOT: ${AAX_SDK_ROOT})

    target_sources(${AX_TARGET}
            PRIVATE
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Alg.h
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_AlgProc.cpp
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Defs.h
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Describe.cpp
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Describe.h
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Parameters.h
            ${AAX_SDK_ROOT}/ExamplePlugins/DemoGain/Source/DemoGain_Parameters.cpp
            ${AAX_SDK_ROOT}/Interfaces/AAX_Exports.cpp

            # ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/wrapasaax_export_entry.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/entry.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/wrapper.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/wrapper.h
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/categories.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/categories.h
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/parameter.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/parameter.h
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/plugview.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/plugview.h
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/process.cpp
            ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/detail/aax/process.h

            # ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/${sd}/src/wrapasaax_export_entry.cpp
            )


    # Define the VST3 plugin name and include the sources directly.
    # We need to indivduate this target since it will be different
    # for different options

    # this creates a individually configured wrapper library for each target
    if (NOT TARGET ${AX_TARGET}-clap-wrapper-aax-lib)
        add_library(${AX_TARGET}-clap-wrapper-aax-lib STATIC)
        private_add_aax_wrapper_sources(TARGET ${AX_TARGET}-clap-wrapper-aax-lib)

        target_link_libraries(${AX_TARGET}-clap-wrapper-aax-lib PUBLIC clap base-sdk-aax)

        # clap-wrapper-extensions are PUBLIC, so a clap linking the library can access the clap-wrapper-extensions
        target_link_libraries(${AX_TARGET}-clap-wrapper-aax-lib PUBLIC
                clap-wrapper-compile-options-public
                clap-wrapper-extensions
                clap-wrapper-shared-detail)
        target_link_libraries(${AX_TARGET}-clap-wrapper-aax-lib PRIVATE clap-wrapper-compile-options)

        target_compile_options(${AX_TARGET}-clap-wrapper-aax-lib PRIVATE
                -DCLAP_SUPPORTS_ALL_NOTE_EXPRESSIONS=$<IF:$<BOOL:${AX_SUPPORTS_ALL_NOTE_EXPRESSIONS}>,1,0>
                )
    endif()

    set_target_properties(${AX_TARGET} PROPERTIES LIBRARY_OUTPUT_NAME "${CLAP_WRAPPER_OUTPUT_NAME}")
    target_link_libraries(${AX_TARGET} PUBLIC ${AX_TARGET}-clap-wrapper-aax-lib )


    if (APPLE)
        if ("${AX_BUNDLE_IDENTIFIER}" STREQUAL "")
            string(REPLACE "_" "-" repout ${outidentifier})
            set(AX_BUNDLE_IDENTIFIER "org.cleveraudio.wrapper.${repout}.aax")
        endif()

        if ("${CLAP_WRAPPER_BUNDLE_VERSION}" STREQUAL "")
            set(CLAP_WRAPPER_BUNDLE_VERSION "1.0")
        endif()

        target_link_libraries (${AX_TARGET} PUBLIC "-framework Foundation" "-framework CoreFoundation")
        set_target_properties(${AX_TARGET} PROPERTIES
                BUNDLE True
                BUNDLE_EXTENSION aax
                LIBRARY_OUTPUT_NAME ${AX_OUTPUT_NAME}
                MACOSX_BUNDLE_GUI_IDENTIFIER ${AX_BUNDLE_IDENTIFIER}
                MACOSX_BUNDLE_BUNDLE_NAME ${AX_OUTPUT_NAME}
                MACOSX_BUNDLE_BUNDLE_VERSION ${AX_BUNDLE_VERSION}
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${AX_BUNDLE_VERSION}
                MACOSX_BUNDLE_INFO_PLIST ${CLAP_WRAPPER_CMAKE_CURRENT_SOURCE_DIR}/cmake/aax_Info.plist.in
                )

        macos_include_clap_in_bundle(TARGET ${AX_TARGET}
                MACOS_EMBEDDED_CLAP_LOCATION ${AX_MACOS_EMBEDDED_CLAP_LOCATION})
        macos_bundle_flag(TARGET ${AX_TARGET})
    endif()
    if(WIN32)
        message(STATUS "clap-wrapper: Building AAX Bundle Folder")
        add_custom_command(TARGET ${AX_TARGET} PRE_BUILD
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<IF:$<CONFIG:Debug>,Debug,Release>/${AX_OUTPUT_NAME}.aaxplugin/Contents/x64"
                )
        set_target_properties(${AX_TARGET} PROPERTIES
                LIBRARY_OUTPUT_NAME ${AX_OUTPUT_NAME}
                LIBRARY_OUTPUT_DIRECTORY "$<IF:$<CONFIG:Debug>,Debug,Release>/${CMAKE_BINARY_DIR}/${AX_OUTPUT_NAME}.aaxplugin/Contents/x64"
                LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${AX_OUTPUT_NAME}.aaxplugin/Contents/x64"
                LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${AX_OUTPUT_NAME}.aaxplugin/Contents/x64"
                SUFFIX ".aaxplugin")

    endif()

    if (${CLAP_WRAPPER_COPY_AFTER_BUILD})
        target_copy_after_build(TARGET ${AX_TARGET} FLAVOR aax)
    endif()
endfunction(target_add_aax_wrapper)
