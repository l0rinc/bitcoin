#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -o errexit -o pipefail -o xtrace

export DEBIAN_FRONTEND=noninteractive
export CI_RETRY_EXE="/ci_retry"

pushd "/"

${CI_RETRY_EXE} apt-get update
# Lint dependencies:
# - cargo (used to run the lint tests)
# - curl/xz-utils (to install shellcheck)
# - git (used in many lint scripts)
# - gpg (used by verify-commits)
# - moreutils (used by scripted-diff)
${CI_RETRY_EXE} apt-get install -y cargo curl xz-utils git gpg moreutils

# Install Python and create venv using uv (reads version from .python-version)
uv venv /python_env

export PATH="/python_env/bin:${PATH}"
command -v python3
python3 --version

uv pip install --python /python_env --requirements /ci/lint/requirements.txt

SHELLCHECK_VERSION=v0.11.0
MLC_VERSION=v1.2.0
case "$(uname --machine)" in
    x86_64)
        SHELLCHECK_ARCH=x86_64
        SHELLCHECK_SHA256=8c3be12b05d5c177a04c29e3c78ce89ac86f1595681cab149b65b97c4e227198
        MLC_ARCH=x86_64
        MLC_SHA256=7a72a93d5b3ee8a554cb840abdfe90aefb709418f225461b52021e3a058238a2
        ;;
    aarch64)
        SHELLCHECK_ARCH=aarch64
        SHELLCHECK_SHA256=12b331c1d2db6b9eb13cfca64306b1b157a86eb69db83023e261eaa7e7c14588
        MLC_ARCH=aarch64
        MLC_SHA256=01ec8e086f3b625616d461b63451be9175a02557de6b591bac7cde6791ab074b
        ;;
    *)
        echo "Unsupported architecture for lint binaries: $(uname --machine)" >&2
        exit 1
        ;;
esac

verify_sha256() {
    local expected="$1" file="$2"
    if ! printf '%s  %s\n' "${expected}" "${file}" | sha256sum --check --status; then
        echo "SHA-256 mismatch for ${file}" >&2
        return 1
    fi
}

lint_asset_dir=$(mktemp -d)
trap 'rm -rf "${lint_asset_dir}"' EXIT

shellcheck_archive="${lint_asset_dir}/shellcheck.tar.xz"
curl --fail -L "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.${SHELLCHECK_ARCH}.tar.xz" \
    --output "${shellcheck_archive}"
verify_sha256 "${SHELLCHECK_SHA256}" "${shellcheck_archive}"
tar --xz -xf "${shellcheck_archive}" --directory "${lint_asset_dir}"
install --mode=755 "${lint_asset_dir}/shellcheck-${SHELLCHECK_VERSION}/shellcheck" /usr/bin/shellcheck

mlc_binary="${lint_asset_dir}/mlc"
curl --fail -L "https://github.com/becheran/mlc/releases/download/${MLC_VERSION}/mlc-${MLC_ARCH}-linux" \
    --output "${mlc_binary}"
verify_sha256 "${MLC_SHA256}" "${mlc_binary}"
install --mode=755 "${mlc_binary}" /usr/bin/mlc

popd || exit
