#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2022,2026 Collabora Inc.
#                         2023 Emmanuele Bassi
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Xavier Claessens <xclaesse@gmail.com>

import argparse
import textwrap
from pathlib import Path


# Disable line length warnings as wrapping the C code templates would be hard
# flake8: noqa: E501


def gen_versions_macros(args, current_major_version, current_minor_version, current_micro_version):
    with args.out_path.open("w", encoding="utf-8") as ofile, args.in_path.open(
        "r", encoding="utf-8"
    ) as ifile:
        ns = args.namespace
        for line in ifile.readlines():
            if "@SHUMATE_VERSIONS@" in line:
                ofile.write(
                    textwrap.dedent(
                        f"""\
                    /**
                     * SHUMATE_MAJOR_VERSION:
                     *
                     * The major version component of the library's version, e.g. "1" for "1.2.3".
                     */
                    #define SHUMATE_MAJOR_VERSION ({current_major_version})

                    /**
                     * SHUMATE_MINOR_VERSION:
                     *
                     * The minor version component of the library's version, e.g. "2" for "1.2.3".
                     */
                    #define SHUMATE_MINOR_VERSION ({current_minor_version})

                    /**
                     * SHUMATE_MICRO_VERSION:
                     *
                     * The micro version component of the library's version, e.g. "3" for "1.2.3".
                     */
                    #define SHUMATE_MICRO_VERSION ({current_micro_version})
                        """
                    )
                )
                for minor in range(0, current_minor_version + 1, 1):
                    ofile.write(
                        textwrap.dedent(
                            f"""\
                        /**
                         * SHUMATE_VERSION_{current_major_version}_{minor}:
                         *
                         * A macro that evaluates to the {current_major_version}.{minor} version of Shumate, in a format
                         * that can be used by the C pre-processor.
                         *
                         * Since: {current_major_version}.{minor}
                         */
                        #define SHUMATE_VERSION_{current_major_version}_{minor} (G_ENCODE_VERSION ({current_major_version}, {minor}))
                        """
                        )
                    )
            elif "@SHUMATE_VISIBILITY@" in line:
                ofile.write(
                    textwrap.dedent(
                        f"""\
                    #pragma once

                    #if (defined(_WIN32) || defined(__CYGWIN__)) && !defined({ns}_STATIC_COMPILATION)
                    #  define _{ns}_EXPORT __declspec(dllexport)
                    #  define _{ns}_IMPORT __declspec(dllimport)
                    #elif __GNUC__ >= 4
                    #  define _{ns}_EXPORT __attribute__((visibility("default")))
                    #  define _{ns}_IMPORT
                    #else
                    #  define _{ns}_EXPORT
                    #  define _{ns}_IMPORT
                    #endif
                    #ifdef {ns}_COMPILATION
                    #  define _{ns}_API _{ns}_EXPORT
                    #else
                    #  define _{ns}_API _{ns}_IMPORT
                    #endif

                    #define _{ns}_EXTERN _{ns}_API extern

                    #define {ns}_VAR _{ns}_EXTERN
                    #define {ns}_AVAILABLE_IN_ALL _{ns}_EXTERN

                    #ifdef SHUMATE_DISABLE_DEPRECATION_WARNINGS
                    #define {ns}_DEPRECATED _{ns}_EXTERN
                    #define {ns}_DEPRECATED_FOR(f) _{ns}_EXTERN
                    #define {ns}_UNAVAILABLE(maj,min) _{ns}_EXTERN
                    #define {ns}_UNAVAILABLE_STATIC_INLINE(maj,min)
                    #else
                    #define {ns}_DEPRECATED G_DEPRECATED _{ns}_EXTERN
                    #define {ns}_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _{ns}_EXTERN
                    #define {ns}_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min) _{ns}_EXTERN
                    #define {ns}_UNAVAILABLE_STATIC_INLINE(maj,min) G_UNAVAILABLE(maj,min)
                    #endif
                    """
                    )
                )
                for minor in range(0, current_minor_version + 1):
                    ofile.write(
                        textwrap.dedent(
                            f"""
                        #if SHUMATE_VERSION_MIN_REQUIRED >= SHUMATE_VERSION_1_{minor}
                        #define {ns}_DEPRECATED_IN_{current_major_version}_{minor} {ns}_DEPRECATED
                        #define {ns}_DEPRECATED_IN_{current_major_version}_{minor}_FOR(f) {ns}_DEPRECATED_FOR (f)
                        #define {ns}_DEPRECATED_MACRO_IN_{current_major_version}_{minor} SHUMATE_DEPRECATED_MACRO
                        #define {ns}_DEPRECATED_MACRO_IN_{current_major_version}_{minor}_FOR(f) SHUMATE_DEPRECATED_MACRO_FOR (f)
                        #define {ns}_DEPRECATED_ENUMERATOR_IN_{current_major_version}_{minor} SHUMATE_DEPRECATED_ENUMERATOR
                        #define {ns}_DEPRECATED_ENUMERATOR_IN_{current_major_version}_{minor}_FOR(f) SHUMATE_DEPRECATED_ENUMERATOR_FOR (f)
                        #define {ns}_DEPRECATED_TYPE_IN_{current_major_version}_{minor} SHUMATE_DEPRECATED_TYPE
                        #define {ns}_DEPRECATED_TYPE_IN_{current_major_version}_{minor}_FOR(f) SHUMATE_DEPRECATED_TYPE_FOR (f)
                        #else
                        #define {ns}_DEPRECATED_IN_{current_major_version}_{minor} _{ns}_EXTERN
                        #define {ns}_DEPRECATED_IN_{current_major_version}_{minor}_FOR(f) _{ns}_EXTERN
                        #define {ns}_DEPRECATED_MACRO_IN_{current_major_version}_{minor}
                        #define {ns}_DEPRECATED_MACRO_IN_{current_major_version}_{minor}_FOR(f)
                        #define {ns}_DEPRECATED_ENUMERATOR_IN_{current_major_version}_{minor}
                        #define {ns}_DEPRECATED_ENUMERATOR_IN_{current_major_version}_{minor}_FOR(f)
                        #define {ns}_DEPRECATED_TYPE_IN_{current_major_version}_{minor}
                        #define {ns}_DEPRECATED_TYPE_IN_{current_major_version}_{minor}_FOR(f)
                        #endif

                        #if SHUMATE_VERSION_MAX_ALLOWED < SHUMATE_VERSION_{current_major_version}_{minor}
                        #define {ns}_AVAILABLE_IN_{current_major_version}_{minor} {ns}_UNAVAILABLE ({current_major_version}, {minor})
                        #define {ns}_AVAILABLE_STATIC_INLINE_IN_{current_major_version}_{minor} SHUMATE_UNAVAILABLE_STATIC_INLINE ({current_major_version}, {minor})
                        #define {ns}_AVAILABLE_MACRO_IN_{current_major_version}_{minor} SHUMATE_UNAVAILABLE_MACRO ({current_major_version}, {minor})
                        #define {ns}_AVAILABLE_ENUMERATOR_IN_{current_major_version}_{minor} SHUMATE_UNAVAILABLE_ENUMERATOR ({current_major_version}, {minor})
                        #define {ns}_AVAILABLE_TYPE_IN_{current_major_version}_{minor} SHUMATE_UNAVAILABLE_TYPE ({current_major_version}, {minor})
                        #else
                        #define {ns}_AVAILABLE_IN_{current_major_version}_{minor} _{ns}_EXTERN
                        #define {ns}_AVAILABLE_STATIC_INLINE_IN_{current_major_version}_{minor}
                        #define {ns}_AVAILABLE_MACRO_IN_{current_major_version}_{minor}
                        #define {ns}_AVAILABLE_ENUMERATOR_IN_{current_major_version}_{minor}
                        #define {ns}_AVAILABLE_TYPE_IN_{current_major_version}_{minor}
                        #endif
                        """
                        )
                    )
            else:
                ofile.write(line)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("shumate_version", help="Current Shumate version")
    parser.add_argument("namespace", help="Macro namespace")
    parser.add_argument("in_path", help="input file", type=Path)
    parser.add_argument("out_path", help="output file", type=Path)

    args = parser.parse_args()
    version = [int(i) for i in args.shumate_version.split(".")]
    gen_versions_macros(args, version[0], version[1], version[2])


if __name__ == "__main__":
    main()
