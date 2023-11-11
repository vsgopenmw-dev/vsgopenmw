# This script re-signs OpenMW.app and OpenMW-CS.app after CPack packages them. This is necessary because CPack modifies
# the library references used by OpenMW to App relative paths, invalidating the code signature.

# Obviously, we only need to run this on Apple targets.
if (APPLE)
    set(APPLICATIONS "OpenMW")
    if (OPENMW_CS)
        list(APPEND APPLICATIONS "OpenMW-CS")
    endif (OPENMW_CS)
    foreach(app_name IN LISTS APPLICATIONS)
        set(FULL_APP_PATH "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/ALL_IN_ONE/${app_name}.app")
        message(STATUS "Re-signing ${app_name}.app")
        execute_process(COMMAND "codesign" "--force" "--deep" "-s" "-" "${FULL_APP_PATH}")
    endforeach(app_name)
endif (APPLE)