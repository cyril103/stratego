# Only the runtime component is packaged; raylib's headers/static libraries
# and development targets must not leak into the installer.
set(STRATEGO_PACKAGE_VERSION "1.0.20260923" CACHE STRING "Debian package version")
install(TARGETS stratego RUNTIME DESTINATION lib/stratego3d COMPONENT Runtime)
install(PROGRAMS "${CMAKE_SOURCE_DIR}/packaging/linux/stratego3d.sh"
        DESTINATION games RENAME stratego3d COMPONENT Runtime)
install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/stratego3d.desktop"
        DESTINATION share/applications COMPONENT Runtime)
install(FILES "${CMAKE_SOURCE_DIR}/assets/ui/imperial-emblem.png"
        DESTINATION share/pixmaps RENAME stratego3d.png COMPONENT Runtime)
foreach(asset_dir backgrounds fonts lighting models pieces shaders textures ui)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/${asset_dir}"
            DESTINATION share/stratego3d/assets COMPONENT Runtime)
endforeach()
install(FILES "${CMAKE_SOURCE_DIR}/docs/LINUX_MINT.md"
        DESTINATION share/doc/stratego3d COMPONENT Runtime)
install(FILES "${STRATEGO_RAYLIB_SOURCE_DIR}/../LICENSE"
        DESTINATION share/doc/stratego3d RENAME raylib-LICENSE COMPONENT Runtime)
install(FILES "${STRATEGO_RAYLIB_SOURCE_DIR}/external/glfw/LICENSE.md"
        DESTINATION share/doc/stratego3d RENAME glfw-LICENSE.md COMPONENT Runtime)
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME stratego3d)
set(CPACK_PACKAGE_VENDOR "Stratego3D project")
set(CPACK_PACKAGE_VERSION "${STRATEGO_PACKAGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Stratego 3D - jeu de strategie contre l'ordinateur")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/cyril103/stratego")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};Stratego3D;Runtime;/")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Stratego3D project")
set(CPACK_DEBIAN_PACKAGE_SECTION games)
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# GLFW/miniaudio load several libraries at runtime: shlibdeps alone cannot
# discover these. Mint 22 uses the t64 ALSA package name.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libx11-6, libxrandr2, libxi6, libxcursor1, libxinerama1, libasound2t64 | libasound2")
include(CPack)
