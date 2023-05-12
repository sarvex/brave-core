#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

"""Script to download rust_deps."""

import argparse
import os
import subprocess
import sys

try:
    from urllib2 import URLError
except ImportError:  # For Py3 compatibility
    from urllib.error import URLError # pylint: disable=no-name-in-module,import-error

import deps
from rust_deps_config import RUST_DEPS_PACKAGES_URL, RUST_DEPS_PACKAGE_VERSION

BRAVE_CORE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

BRAVE_BROWSER_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

RUSTUP_PATH = os.path.join(BRAVE_CORE_ROOT, 'build', 'rustup')

RUSTUP_HOME = os.path.join(RUSTUP_PATH, RUST_DEPS_PACKAGE_VERSION)


def get_url(platform):
    if platform in ["win32", "cygwin"]:
        filename = f"rust_deps_win_{RUST_DEPS_PACKAGE_VERSION}.zip"
    elif platform == 'darwin':
        filename = f"rust_deps_mac_{RUST_DEPS_PACKAGE_VERSION}.gz"
    elif platform == 'ios':
        filename = f"rust_deps_ios_{RUST_DEPS_PACKAGE_VERSION}.gz"
    elif platform.startswith('linux'):
        filename = f"rust_deps_linux_{RUST_DEPS_PACKAGE_VERSION}.gz"
    else:
        print(f'Invalid platform for Rust deps: {platform}')
        print('Exiting.')
        sys.exit(1)

    return f"{RUST_DEPS_PACKAGES_URL}/{filename}"


def should_download():
    return not os.path.exists(RUSTUP_HOME)


def download_and_unpack_rust_deps(platform):
    if not should_download():
        return 0

    url = get_url(platform)

    try:
        deps.DownloadAndUnpack(url, RUSTUP_PATH)
    except URLError:
        print(f'Failed to download Rust deps: {url}')
        print('Exiting.')
        sys.exit(1)


def get_android_api_level(target_arch):
    return {
        'arm': '19',
        'arm64': '24',
        'x86': '19',
        'x86_64': '24',
    }[target_arch]


def get_android_target(target_arch):
    return {
        'arm': 'arm-linux-androideabi',
        'arm64': 'aarch64-linux-android',
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }[target_arch]


def get_android_linker(target_arch):
    return f"{get_android_target(target_arch)}-clang"


def make_standalone_toolchain_for_android():
    ANDROID_NDK_PATH = os.path.join(
        BRAVE_BROWSER_ROOT, 'third_party', 'android_ndk')

    make_standalone_toolchain = os.path.join(
        ANDROID_NDK_PATH, 'build', 'tools', 'make_standalone_toolchain.py')

    config_path = os.path.join(RUSTUP_HOME, 'config')
    with open(config_path, "w+") as fp:
        for target_arch in ['arm', 'arm64', 'x86', 'x86_64']:
            toolchain_path = os.path.join(RUSTUP_PATH, 'toolchains', target_arch)

            api_level = get_android_api_level(target_arch)

                    # Make standalone Android toolchain for target_arch
            toolchain_args = [
                make_standalone_toolchain,
                "--force",
                f"--install-dir={toolchain_path}",
                f"--api={api_level}",
                f"--arch={target_arch}",
            ]
            try:
                subprocess.check_call(toolchain_args, env=None)
            except subprocess.CalledProcessError as e:
                print(e.output)
                raise e

                    # Add target to rustup config
            fp.write(f"[target.{get_android_target(target_arch)}" + "]\r\n")
            fp.write("linker = \"" + get_android_linker(target_arch) + "\"\r\n")


def parse_args():
    parser = argparse.ArgumentParser(description='Download rust deps')

    parser.add_argument('--platform')

    return parser.parse_args()


def cargo_install(tool):
    # Set environment variables for rustup
    env = os.environ.copy()
    env['RUSTUP_HOME'] = RUSTUP_HOME
    env['CARGO_HOME'] = RUSTUP_HOME

    rustup_bin = os.path.abspath(os.path.join(RUSTUP_HOME, 'bin'))
    cargo_bin = os.path.join(
        rustup_bin, "cargo" if sys.platform != "win32" else "cargo.exe")

    # Install the tool
    cargo_args = [cargo_bin, "install", tool["name"]]
    if "path" in tool:
        cargo_args.append("--path")
        cargo_args.extend((tool["path"], "--target-dir"))
        cargo_args.append(os.path.join(RUSTUP_HOME, ".build"))
    if "version" in tool:
        cargo_args.append("--version")
        cargo_args.append(tool["version"])
    if "features" in tool:
        cargo_args.append("--features")
        cargo_args.append(tool["features"])
    cargo_args.append("--locked")

    try:
        subprocess.check_call(cargo_args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def main():
    args = parse_args()

    if args.platform == 'ios':
        download_and_unpack_rust_deps('ios')
    else:
        download_and_unpack_rust_deps(sys.platform)

    if args.platform == 'android':
        make_standalone_toolchain_for_android()

    cxx_path = os.path.abspath(
        os.path.join(BRAVE_CORE_ROOT, '..', 'third_party', 'rust', 'cxx', 'v1'))

    with open(os.path.join(cxx_path, "README.chromium")) as readme_file:
        _VERSION_PREFIX = "Version: "
        for line in readme_file:
            if not line.startswith(_VERSION_PREFIX):
                continue
            cxx_version = line[len(_VERSION_PREFIX):].strip()

    tools = [
        {
            "name": "cbindgen",
            "version": "0.14.2",
        },
        {
            "name": "cxxbridge-cmd",
            "version": cxx_version,
        },
        {
            "name": "cargo-audit",
            "features": "vendored-openssl",
        }
    ]
    for tool in tools:
        cargo_install(tool)

    return 0


if __name__ == '__main__':
    sys.exit(main())
