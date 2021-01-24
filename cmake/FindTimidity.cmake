#[=======================================================================[.rst:
FindTimidity
-------

Finds the Timidity library.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

`Timidity_FOUND``
  True if the system has the Timidity library.
``Timidity_INCLUDE_DIRS``
  Include directories needed to use Timidity.
``Timidity_LIBRARIES``
  Libraries needed to link to Timidity.
``Timidity_VERSION``
  The version of the Timidity library which was found.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Timidity_INCLUDE_DIR``
  The directory containing ``timidity.h``.
``Timidity_LIBRARY``
  The path to the Timidity library.

#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_Timidity QUIET timidity)

find_path(Timidity_INCLUDE_DIR
    NAMES timidity.h
    PATHS ${PC_Timidity_INCLUDE_DIRS}
)

find_library(Timidity_LIBRARY
    NAMES timidity
    PATHS ${PC_Timidity_LIBRARY_DIRS}
)

set(Timidity_VERSION ${PC_Timidity_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Timidity
    FOUND_VAR Timidity_FOUND
    REQUIRED_VARS
        Timidity_LIBRARY
        Timidity_INCLUDE_DIR
    VERSION_VAR Timidity_VERSION
)

if(Timidity_FOUND)
    set(Timidity_LIBRARIES ${Timidity_LIBRARY})
    set(Timidity_INCLUDE_DIRS ${Timidity_INCLUDE_DIR})
endif()

mark_as_advanced(
    Timidity_INCLUDE_DIR
    Timidity_LIBRARY
)
