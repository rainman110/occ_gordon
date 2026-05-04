#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
#

function(occ_gordon_detect_installed_pythonocc_version python_executable out_version)
  execute_process(
    COMMAND ${python_executable} -c "import OCC; print('%d.%d.%d' % (OCC.PYTHONOCC_VERSION_MAJOR, OCC.PYTHONOCC_VERSION_MINOR, OCC.PYTHONOCC_VERSION_PATCH))"
    OUTPUT_VARIABLE _pythonocc_installed_version
    ERROR_VARIABLE _pythonocc_installed_version_error
    RESULT_VARIABLE _pythonocc_installed_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (NOT _pythonocc_installed_version_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to detect installed pythonocc version from interpreter '${python_executable}':\n${_pythonocc_installed_version_error}")
  endif()

  set(${out_version} "${_pythonocc_installed_version}" PARENT_SCOPE)
endfunction()


function(occ_gordon_resolve_pythonocc_source_dir)
  set(options)
  set(oneValueArgs PYTHONOCC_INSTALLED_VERSION)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "" ${ARGN})

  if (NOT PythonOCC_SOURCE_DIR AND OCC_GORDON_FETCH_PYTHONOCC_SOURCE)
    include(FetchContent)

    if (OCC_GORDON_PYTHONOCC_GIT_TAG)
      set(_occ_gordon_pythonocc_git_tag "${OCC_GORDON_PYTHONOCC_GIT_TAG}")
    else()
      set(_occ_gordon_pythonocc_git_tag "${ARG_PYTHONOCC_INSTALLED_VERSION}")
    endif()

    message(STATUS
      "PythonOCC_SOURCE_DIR not provided. Fetching pythonocc-core sources from "
      "${OCC_GORDON_PYTHONOCC_REPOSITORY} at tag '${_occ_gordon_pythonocc_git_tag}'")

    FetchContent_Declare(
      pythonocc_core_sources
      GIT_REPOSITORY ${OCC_GORDON_PYTHONOCC_REPOSITORY}
      GIT_TAG ${_occ_gordon_pythonocc_git_tag}
      GIT_SHALLOW TRUE
    )

    FetchContent_GetProperties(pythonocc_core_sources)
    if (NOT pythonocc_core_sources_POPULATED)
      FetchContent_Populate(pythonocc_core_sources)
    endif()

    set(PythonOCC_SOURCE_DIR "${pythonocc_core_sources_SOURCE_DIR}" CACHE PATH
        "Path to pythonocc-core source tree (must contain src/SWIG_files/wrapper/Standard.i)" FORCE)
    message(STATUS "Fetched pythonocc-core sources to: ${PythonOCC_SOURCE_DIR}")
  endif()
endfunction()


function(occ_gordon_validate_swig_compatibility python_executable)
  execute_process(
    COMMAND ${python_executable} -c "import pathlib,re,OCC.Core._Geom as G; b=pathlib.Path(G.__file__).read_bytes(); m=re.search(rb'swig_runtime_data([0-9]+)', b); print(m.group(1).decode() if m else '')"
    OUTPUT_VARIABLE _pythonocc_swig_runtime_abi
    ERROR_VARIABLE _pythonocc_swig_runtime_abi_error
    RESULT_VARIABLE _pythonocc_swig_runtime_abi_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (NOT _pythonocc_swig_runtime_abi_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to detect SWIG runtime ABI used by pythonocc binaries from interpreter '${python_executable}':\n${_pythonocc_swig_runtime_abi_error}")
  endif()

  if (NOT _pythonocc_swig_runtime_abi)
    message(FATAL_ERROR
      "Could not detect SWIG runtime ABI marker (swig_runtime_dataN) in OCC.Core._Geom. "
      "Set OCC_GORDON_SKIP_SWIG_VERSION_CHECK=ON to bypass.")
  endif()

  execute_process(
    COMMAND ${SWIG_EXECUTABLE} -swiglib
    OUTPUT_VARIABLE _swig_lib_dir
    ERROR_VARIABLE _swig_lib_dir_error
    RESULT_VARIABLE _swig_lib_dir_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (NOT _swig_lib_dir_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to query SWIG library directory from '${SWIG_EXECUTABLE}':\n${_swig_lib_dir_error}")
  endif()

  set(_swig_run_file "${_swig_lib_dir}/swigrun.swg")
  if (NOT EXISTS "${_swig_run_file}")
    message(FATAL_ERROR
      "Could not find '${_swig_run_file}' to determine SWIG runtime ABI.")
  endif()

  file(READ "${_swig_run_file}" _swig_run_file_content)
  string(REGEX MATCH "#define SWIG_RUNTIME_VERSION \"([0-9]+)\"" _swig_runtime_version_match "${_swig_run_file_content}")
  if (NOT CMAKE_MATCH_1)
    message(FATAL_ERROR
      "Could not parse SWIG runtime ABI from '${_swig_run_file}'.")
  endif()
  set(_swig_runtime_abi "${CMAKE_MATCH_1}")

  message(STATUS "pythonocc SWIG runtime ABI: ${_pythonocc_swig_runtime_abi}")
  message(STATUS "Configured SWIG runtime ABI: ${_swig_runtime_abi}")
  message(STATUS "Configured SWIG executable version: ${SWIG_VERSION}")

  if (NOT _swig_runtime_abi STREQUAL _pythonocc_swig_runtime_abi)
    message(FATAL_ERROR
      "SWIG runtime ABI mismatch detected.\n"
      "  pythonocc binary uses swig_runtime_data${_pythonocc_swig_runtime_abi}\n"
      "  configured SWIG uses runtime ABI ${_swig_runtime_abi} (${SWIG_EXECUTABLE})\n"
      "Use a SWIG executable with matching runtime ABI to avoid runtime type-conversion failures.")
  endif()

  execute_process(
    COMMAND ${python_executable} -c "import pathlib,re,OCC.Core.Geom as G; p=pathlib.Path(G.__file__); t=p.read_text(encoding='utf-8',errors='ignore'); m=re.search(r'^# Version ([0-9]+\\.[0-9]+\\.[0-9]+)', t, re.M); print(m.group(1) if m else '')"
    OUTPUT_VARIABLE _pythonocc_swig_version
    ERROR_VARIABLE _pythonocc_swig_version_error
    RESULT_VARIABLE _pythonocc_swig_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (NOT _pythonocc_swig_version_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to detect SWIG version used by pythonocc wrappers from interpreter '${python_executable}':\n${_pythonocc_swig_version_error}")
  endif()

  if (NOT _pythonocc_swig_version)
    message(WARNING
      "Could not parse SWIG version header from OCC.Core.Geom wrapper. "
      "Runtime ABI check passed, continuing without exact-version information.")
    return()
  endif()

  message(STATUS "pythonocc wrapper SWIG version: ${_pythonocc_swig_version}")
  if (OCC_GORDON_ENFORCE_EXACT_SWIG_VERSION AND NOT SWIG_VERSION VERSION_EQUAL _pythonocc_swig_version)
    message(FATAL_ERROR
      "Exact SWIG version mismatch detected.\n"
      "  pythonocc wrappers were generated with SWIG ${_pythonocc_swig_version}\n"
      "  configured SWIG executable is ${SWIG_VERSION} (${SWIG_EXECUTABLE})\n"
      "Disable OCC_GORDON_ENFORCE_EXACT_SWIG_VERSION or use the exact SWIG version.")
  endif()
endfunction()
