vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/MathisRosenhauer/libaec/releases/download/v${VERSION}/libaec-${VERSION}.tar.gz"
    FILENAME "libaec-${VERSION}.tar.gz"
    SHA512 36fe7a264b3308f82050383843559640fe4e62cf89d44385ceaa79fff0368f72b041832726a7cf9e6ca367fe9bae33d5c2a05690f616c23d79a9db93bc2b6f7f
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    PATCHES
        static-shared.patch
        cmake-config.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTING=OFF
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(CONFIG_PATH "cmake")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CURRENT_PORT_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
