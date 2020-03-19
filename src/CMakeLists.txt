# The ability to start with an empty target was added in CMake 3.11 so since
# for Android we use a shared library we need to use a macro to determine which
# function to call.
macro(initial_sources TARGET)
	if(ANDROID)
		add_library(engine SHARED ${ARGV})
	else()
		add_executable(engine WIN32 ${ARGV})
	endif()
endmacro()

initial_sources(
	id_ca.cpp
	id_in.cpp
	id_pm.cpp
	id_sd.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
        signon.cpp
	wl_act1.cpp
	wl_act2.cpp
	wl_agent.cpp
	wl_debug.cpp
	wl_draw.cpp
	wl_game.cpp
	wl_inter.cpp
	wl_main.cpp
	wl_menu.cpp
	wl_play.cpp
	wl_state.cpp
	wl_text.cpp
)

include(CheckCXXSourceCompiles)
include(CheckFunctionExists)
include(FindPkgConfig)

target_sources(engine PRIVATE ../mame/fmopl.cpp)

check_function_exists(stricmp STRICMP_EXISTS)
check_function_exists(strnicmp STRNICMP_EXISTS)
check_function_exists(atoll ATOLL_EXISTS)
if(NOT STRICMP_EXISTS)
	target_compile_definitions(engine PRIVATE -Dstricmp=strcasecmp)
endif()
if(NOT STRNICMP_EXISTS)
	target_compile_definitions(engine PRIVATE -Dstrnicmp=strncasecmp)
endif()
if(NOT ATOLL_EXISTS)
	target_compile_definitions(engine PRIVATE -Datoll=_atoi64)
endif()

check_cxx_source_compiles(
	"#include <stdarg.h>
	int main() { va_list list1, list2; va_copy(list1, list2); return 0; }"
	HAS_VA_COPY )
if( NOT HAS_VA_COPY )
	check_cxx_source_compiles(
		"#include <stdarg.h>
		int main() { va_list list1, list2; __va_copy(list1, list2); return 0; }"
		HAS___VA_COPY )
	if( HAS___VA_COPY )
		target_compile_definitions(engine PRIVATE -Dva_copy=__va_copy)
	else()
		target_compile_definitions(engine PRIVATE -DNO_VA_COPY)
	endif()
endif()

if( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 )
		target_compile_options(engine PRIVATE -Wno-implicit-fallthrough)
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4 )
		target_compile_options(engine PRIVATE -Wno-unused-result)
	endif()
	target_compile_options(engine PRIVATE -Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

if(WIN32)
	set(NO_GTK ON)
	target_link_libraries(engine PRIVATE comctl32)
	target_compile_definitions(engine PRIVATE -DNO_STDIO_REDIRECT)

	# RC file
	target_sources(engine PRIVATE win32/windows.rc)

	# Fix stat in v140_xp (broken in RTM and Update 1 so far)
	if( MSVC AND MSVC_VERSION EQUAL 1900 AND CMAKE_GENERATOR_TOOLSET STREQUAL "v140_xp" )
		target_compile_definitions(engine PRIVATE -D_stat64i32=VS14Stat)
		target_sources(engine PRIVATE win32/vs2015hack.cpp)
	endif()

	target_sources(engine PRIVATE
		win32/i_crash.cpp
		win32/i_main.cpp
		win32/wl_iwad_win32.cpp
	)
elseif(APPLE OR ANDROID)
	set(NO_GTK ON)
else()
	option( NO_GTK "Disable GTK+ dialogs (Not applicable to Windows)" )

	# Use GTK+ for the IWAD picker, if available.
	if( NOT NO_GTK )
		pkg_check_modules( GTK3 gtk+-3.0 )
		if( GTK3_FOUND )
			target_link_libraries(engine PRIVATE ${GTK3_LIBRARIES})
			target_include_directories(engine PRIVATE ${GTK3_INCLUDE_DIRS})
		else()
			set( NO_GTK ON )
		endif()
	endif()
endif()

if(NO_GTK)
	target_compile_definitions(engine PRIVATE -DNO_GTK=1)
endif()

if(APPLE)
        # target_sources(engine PRIVATE ../MACOSX/SDLMain.m)

	option(BUILD_BUNDLE "Build a app bundle on Mac OS X" OFF)
	if(BUILD_BUNDLE)
		set_target_properties(engine PROPERTIES MACOSX_BUNDLE ON)
	endif(BUILD_BUNDLE)
elseif(ANDROID)
	target_sources(engine PRIVATE
		android/android-jni.cpp
		android/in_android.cpp
		android/wl_iwad_picker_android.cpp
	)

	# Aarch64 linker seems to be setup improperly and requires these to be
 	# public for link-test
	target_link_libraries(engine PUBLIC
		touchcontrols
		GLESv1_CM
		log
	)
endif()

# Derive version info and generate header
string(TOUPPER "${PRODUCT_NAME}" PRODUCT_SIGNATURE)
string(TOLOWER "${PRODUCT_NAME}" PRODUCT_BINARY)

# Glob for the header files so project generation can include them
file(GLOB HEADER_FILES
	*.h
)
target_sources(engine PRIVATE ${HEADER_FILES})

target_link_libraries(engine PRIVATE
  SDL2::SDL2_mixer
  SDL2::SDL2
)
target_include_directories(engine PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(engine PROPERTIES
	CXX_STANDARD 98
	OUTPUT_NAME "${ENGINE_BINARY_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	# On Linux look for any libraries we link against in the executable directory.
	INSTALL_RPATH "\$ORIGIN"
	BUILD_WITH_INSTALL_RPATH ON
)

# Install
if(NOT ANDROID)
	install(TARGETS engine BUNDLE DESTINATION ${OUTPUT_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
endif()

# Project file source groupings
source_group("Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/.+")
source_group("OPL Emulator\\MAME" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mame/.+")
