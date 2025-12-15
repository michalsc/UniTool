# Copyright © 2020 Michal Schulz <michal.schulz@gmx.de>
# https://github.com/michalsc
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

function(add_catalog CATALOG_NAME OUTPUT_NAME)
    
    # Use flexcat to build header file
    add_custom_command(
        OUTPUT includes/${OUTPUT_NAME}
        COMMAND flexcat "${CMAKE_CURRENT_SOURCE_DIR}/${CATALOG_NAME}"
                        includes/${OUTPUT_NAME}=${CMAKE_SOURCE_DIR}/templates/C_h_aros.sd || [ $$? -lt 10 ]
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${CATALOG_NAME}
    )

    # Create virtual catalogs target. Link with it in order to access the
    # generated header files.
    # If target already exists, add header as its private source so that it
    # is build
    if(TARGET catalogs)
        target_sources(catalogs PRIVATE includes/${OUTPUT_NAME})
    else()
        add_library(catalogs INTERFACE includes/${OUTPUT_NAME})
        target_include_directories(catalogs INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/includes)
    endif()

    # Scan for translations for the component.
    get_filename_component(SCAN_PATH ${CATALOG_NAME} DIRECTORY)
    if (SCAN_PATH STREQUAL "")
        set(SCAN_PATH ".")
    endif()
    file(GLOB LANGS ${SCAN_PATH}/*.ct)

    get_filename_component(CAT ${CATALOG_NAME} NAME_WLE)

    # Build all translations using the catalog name as file base and language as directory
    foreach(LANGUAGE_FILE IN LISTS LANGS)
        get_filename_component(LANGUAGE ${LANGUAGE_FILE} NAME_WLE)
        file(MAKE_DIRECTORY ${LANGUAGE})
        add_custom_command(
            OUTPUT ${LANGUAGE}/${CAT}.catalog
            COMMAND flexcat "${CMAKE_CURRENT_SOURCE_DIR}/${CATALOG_NAME}"
                            ${LANGUAGE_FILE}
                            CATALOG="${LANGUAGE}/${CAT}.catalog" || [ $$? -lt 10 ]
            COMMAND echo "${CMAKE_CURRENT_SOURCE_DIR}/${CATALOG_NAME}"
            COMMAND echo "${LANGUAGE_FILE} CATALOG=${LANGUAGE}/${CAT}.catalog"
            DEPENDS ${LANGUAGE_FILE}
                    ${CATALOG_NAME}
        )
        
        # Make install target
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LANGUAGE}/${CAT}.catalog 
                DESTINATION ./Catalogs/${LANGUAGE})

        # Add translations as private sources to catalogs target
        target_sources(catalogs PRIVATE ${LANGUAGE}/${CAT}.catalog)
    endforeach()

endfunction(add_catalog)
