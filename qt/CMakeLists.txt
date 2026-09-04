cmake_minimum_required(VERSION 3.17)
project(nand_programmer)

# Set the CMAKE_PREFIX_PATH to find Qt5
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "/opt/local/libexec/qt5/lib/cmake/Qt5" ${CMAKE_PREFIX_PATH})
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)

# Find Qt5 Widgets
find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort)

# Enable Qt's AUTOMOC (meta-object compiler) feature
set(CMAKE_AUTOMOC ON)
# Enable AUTOMOC (to process Qt's meta-object code) and AUTOUIC (to process .ui files)
set(CMAKE_AUTOUIC ON)

# Include directories (Qt headers should be found automatically)
include_directories(.)

# Set Boost root directory
if(NOT BOOST_ROOT)
set(BOOST_ROOT "/opt/local")
set(BOOST_INCLUDEDIR "/opt/local/include")
set(BOOST_LIBRARYDIR "/opt/local/lib")
endif()

find_package(Boost REQUIRED COMPONENTS system thread)

# If you use Boost headers
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR} ${Qt5Core_INCLUDE_DIRS} ${Qt5SerialPort_INCLUDE_DIRS})
file(GLOB APP_SOURCES ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.ui
        ${CMAKE_SOURCE_DIR}/ecc/*.cpp)
if (APPLE)
    add_executable(nand_programmer
            MACOSX_BUNDLE  # Specify this flag to create a macOS app bundle
            ${APP_SOURCES}
            )
else ()
    add_executable(nand_programmer
            ${APP_SOURCES}
            )
endif ()
# Copy CSV files into the build directory
file(COPY ${CMAKE_SOURCE_DIR}/nando_parallel_chip_db.csv DESTINATION ${CMAKE_BINARY_DIR}/)
file(COPY ${CMAKE_SOURCE_DIR}/nando_spi_chip_db.csv DESTINATION ${CMAKE_BINARY_DIR}/)
file(COPY ${CMAKE_SOURCE_DIR}/nando_parallel_serial_chip_db.csv DESTINATION ${CMAKE_BINARY_DIR}/)

# Link with Qt5 Widgets
target_link_libraries(nand_programmer Qt5::Widgets Qt5::SerialPort Boost::system Boost::thread)


if (APPLE)
    set_target_properties(nand_programmer PROPERTIES
            MACOSX_BUNDLE_ICON_FILE app_icon.icns  # Optional: Set app icon (replace with your .icns file)
            MACOSX_BUNDLE_GUI_IDENTIFIER com.nando.nando
            MACOSX_BUNDLE_BUNDLE_VERSION 1.0
            MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0
            )
    if(NOT MACDEPLOYQT_PATH)
        set(MACDEPLOYQT_PATH "/opt/local/libexec/qt5/bin/macdeployqt")
    endif()

    file(COPY ${CMAKE_SOURCE_DIR}/nando_parallel_chip_db.csv
            DESTINATION ${CMAKE_BINARY_DIR}/nand_programmer.app/Contents/Resources)

    file(COPY ${CMAKE_SOURCE_DIR}/nando_spi_chip_db.csv
            DESTINATION ${CMAKE_BINARY_DIR}/nand_programmer.app/Contents/Resources)

    file(COPY ${CMAKE_SOURCE_DIR}/nando_parallel_serial_chip_db.csv
            DESTINATION ${CMAKE_BINARY_DIR}/nand_programmer.app/Contents/Resources)



    # Set the path to your icon file
    set(MACOS_APP_ICON ${CMAKE_SOURCE_DIR}/img/app_icon.icns)

    file(COPY ${MACOS_APP_ICON} DESTINATION ${CMAKE_BINARY_DIR}/nand_programmer.app/Contents/Resources)


    # --- Combined POST_BUILD: macdeployqt + code sign + DMG ---
    # CMake allows only one POST_BUILD per target, so chain all commands
    find_program(CREATE_DMG create-dmg)

    if(CREATE_DMG)
        set(DMG_STAGE_DIR ${CMAKE_BINARY_DIR}/dmg_staging)
        add_custom_command(TARGET nand_programmer POST_BUILD
                COMMAND ${MACDEPLOYQT_PATH} ${CMAKE_BINARY_DIR}/nand_programmer.app
                COMMAND codesign -f -s - --timestamp --deep ${CMAKE_BINARY_DIR}/nand_programmer.app
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${DMG_STAGE_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${DMG_STAGE_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/nand_programmer.app ${DMG_STAGE_DIR}/nand_programmer.app
                COMMAND ${CREATE_DMG} --volname "Nand Programmer"
                        --window-size 500 300
                        --app-drop-link 280 150
                        --no-internet-enable
                        --skip-jenkins
                        ${CMAKE_BINARY_DIR}/nand_programmer.dmg
                        ${DMG_STAGE_DIR}
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${DMG_STAGE_DIR}
                COMMENT "Bundle Qt libs → sign → create DMG"
                )
    else()
        add_custom_command(TARGET nand_programmer POST_BUILD
                COMMAND ${MACDEPLOYQT_PATH} ${CMAKE_BINARY_DIR}/nand_programmer.app
                COMMAND codesign -f -s - --timestamp --deep ${CMAKE_BINARY_DIR}/nand_programmer.app
                COMMAND hdiutil create -format UDZO -srcfolder ${CMAKE_BINARY_DIR}/nand_programmer.app
                        -volname "Nand Programmer"
                        ${CMAKE_BINARY_DIR}/nand_programmer.dmg
                COMMENT "Bundle Qt libs → sign → create DMG (hdiutil)"
                )
    endif()
endif ()
