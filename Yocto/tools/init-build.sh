#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "[ERROR] Source this script: source tools/init-build.sh [build-directory]" >&2
    exit 1
fi

# ============================================================
# Yocto project configuration
# ============================================================

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" || return 1
PROJECT_DIR="$(dirname "${TOOLS_DIR}")"
POKY_DIR="${PROJECT_DIR}/poky"

BRANCH="scarthgap"
BUILD_DIR="${1:-build}"
# The layer template expects the build directory directly under PROJECT_DIR.
if [[ "${BUILD_DIR}" == */* || "${BUILD_DIR}" == "." || "${BUILD_DIR}" == ".." ]]; then
    echo "[ERROR] Build directory must be a directory name directly under ${PROJECT_DIR}." >&2
    return 1
fi

# Pinned commits verified by the project
POKY_COMMIT="4fb7016a07507683359ebd620f1951fd5ea44bfa"
META_OE_COMMIT="0f00f8b9a21950640da8c5707343e5540133f86e"
META_RPI_COMMIT="018d44467c4f09eb63f8ea5e86e85d755250cf89"
META_QT5_COMMIT="916d37e38077850ff5a0ac4fce6f2a55ba5b2a86"

SETUP_BBLAYERS_SCRIPT="${TOOLS_DIR}/setup-bblayers.sh"
SETUP_BUILD_SCRIPT="${TOOLS_DIR}/setup-build.sh"

# ============================================================
# Helper functions
# ============================================================

die() {
    echo "[ERROR] $*" >&2
    return 1
}

ensure_repo() {
    local repo_dir="$1"
    local repo_url="$2"
    local target_commit="$3"
    local repo_name
    local actual_commit
    local origin_url
    local worktree_status actual_branch branch_commit candidate

    repo_name="$(basename "${repo_dir}")"

    echo
    echo "[INFO] Ensuring ${repo_name} is at pinned commit..."

    # --------------------------------------------------------
    # 1. Clone the selected branch if repository is missing
    # --------------------------------------------------------
    if [[ ! -e "${repo_dir}/.git" ]]; then
        if [[ -e "${repo_dir}" ]]; then
            die "${repo_dir} exists but is not a Git repository. Inspect or back up it first."
            return 1
        fi

        echo "[INFO] Cloning branch ${BRANCH}..."

        git clone \
            --branch "${BRANCH}" \
            --single-branch \
            "${repo_url}" "${repo_dir}" || {
                die "Failed to clone ${repo_name}."
                return 1
            }
    else
        # ----------------------------------------------------
        # 2. Never discard local modifications automatically
        # ----------------------------------------------------
        worktree_status="$(git -C "${repo_dir}" status --porcelain --untracked-files=all)" || return 1
        if [[ -n "${worktree_status}" ]]; then
            die "${repo_name} has local changes or untracked files. Back them up or clean them before retrying."
            return 1
        fi

        # ----------------------------------------------------
        # 3. Verify the configured remote
        # ----------------------------------------------------
        origin_url="$(git -C "${repo_dir}" remote get-url origin)" || {
            die "${repo_name} has no usable origin remote."
            return 1
        }

        if [[ "${origin_url}" != "${repo_url}" ]]; then
            die "${repo_name} origin URL does not match the expected URL.
Expected: ${repo_url}
Actual:   ${origin_url}"
            return 1
        fi

        # ----------------------------------------------------
        # 4. Fetch the latest branch history from origin
        #    Refresh the remote-tracking ref even if it moved
        # ----------------------------------------------------
        echo "[INFO] Fetching origin/${BRANCH}..."

        git -C "${repo_dir}" fetch origin \
            "+refs/heads/${BRANCH}:refs/remotes/origin/${BRANCH}" || {
                die "Failed to fetch ${repo_name} branch ${BRANCH}."
                return 1
            }
    fi

    # --------------------------------------------------------
    # 5. Ensure the pinned commit is available locally
    # --------------------------------------------------------
    if ! git -C "${repo_dir}" cat-file -e \
        "${target_commit}^{commit}" 2>/dev/null; then

        echo "[INFO] Pinned commit is missing locally; fetching it..."

        git -C "${repo_dir}" fetch origin "${target_commit}" || {
            die "Cannot fetch pinned commit ${target_commit} for ${repo_name}."
            return 1
        }
    fi

    # --------------------------------------------------------
    # 6. Verify the pinned commit belongs to scarthgap
    # --------------------------------------------------------
    if ! git -C "${repo_dir}" merge-base --is-ancestor \
        "${target_commit}" "refs/remotes/origin/${BRANCH}"; then

        die "Pinned commit ${target_commit} is not verified as part of origin/${BRANCH} for ${repo_name}."
        return 1
    fi

    # --------------------------------------------------------
    # 7. Automatically restore the exact pinned commit
    # --------------------------------------------------------
    actual_commit="$(git -C "${repo_dir}" rev-parse HEAD)" || return 1

    actual_branch="$(git -C "${repo_dir}" symbolic-ref --quiet --short HEAD)" || actual_branch=""
    branch_commit="$(git -C "${repo_dir}" rev-parse --verify "refs/heads/${BRANCH}" 2>/dev/null)" || branch_commit=""

    # Refuse to move a local branch or leave a detached commit that is not
    # reachable from the fetched upstream branch. A clean tree alone is not
    # enough to protect local commits.
    for candidate in "${branch_commit}"; do
        if [[ -n "${candidate}" ]] && ! git -C "${repo_dir}" merge-base --is-ancestor \
            "${candidate}" "refs/remotes/origin/${BRANCH}"; then
            die "${repo_name}: local ${BRANCH} contains commits outside origin/${BRANCH}. Preserve or reconcile them before retrying."
            return 1
        fi
    done
    if [[ -z "${actual_branch}" ]] && ! git -C "${repo_dir}" merge-base --is-ancestor \
        "${actual_commit}" "refs/remotes/origin/${BRANCH}"; then
        die "${repo_name}: detached HEAD contains commits outside origin/${BRANCH}. Create a backup branch before retrying."
        return 1
    fi

    if [[ "${actual_commit}" != "${target_commit}" || "${actual_branch}" != "${BRANCH}" ]]; then
        echo "[INFO] Setting ${repo_name} ${BRANCH} to ${target_commit}..."
        git -C "${repo_dir}" checkout -B "${BRANCH}" "${target_commit}" || {
            die "Failed to checkout pinned commit for ${repo_name}."
            return 1
        }
    else
        echo "[INFO] ${repo_name} is already at the pinned commit."
    fi

    # --------------------------------------------------------
    # 8. Verify the final state; fail if anything is incorrect
    # --------------------------------------------------------
    actual_commit="$(git -C "${repo_dir}" rev-parse HEAD)" || return 1

    if [[ "${actual_commit}" != "${target_commit}" ]]; then
        die "${repo_name} commit verification failed.
Expected: ${target_commit}
Actual:   ${actual_commit}"
        return 1
    fi

    actual_branch="$(git -C "${repo_dir}" symbolic-ref --quiet --short HEAD)" || return 1
    if [[ "${actual_branch}" != "${BRANCH}" ]]; then
        die "${repo_name} branch verification failed: ${actual_branch}"
        return 1
    fi
    worktree_status="$(git -C "${repo_dir}" status --porcelain --untracked-files=all)" || return 1
    if [[ -n "${worktree_status}" ]]; then
        die "${repo_name} working tree is not clean after checkout."
        return 1
    fi

    echo "[OK] ${repo_name}"
    echo "     Branch: ${actual_branch}"
    echo "     Commit: ${actual_commit}"
}

# ============================================================
# Validate project setup scripts
# ============================================================

if [[ ! -f "${SETUP_BBLAYERS_SCRIPT}" ]]; then
    die "Missing script: ${SETUP_BBLAYERS_SCRIPT}"
    return 1 2>/dev/null || exit 1
fi

if [[ ! -f "${SETUP_BUILD_SCRIPT}" ]]; then
    die "Missing script: ${SETUP_BUILD_SCRIPT}"
    return 1 2>/dev/null || exit 1
fi

# ============================================================
# Ensure all repositories use the pinned commits
# ============================================================

ensure_repo \
    "${POKY_DIR}" \
    "https://git.yoctoproject.org/poky" \
    "${POKY_COMMIT}" || {
        return 1 2>/dev/null || exit 1
    }

ensure_repo \
    "${PROJECT_DIR}/meta-openembedded" \
    "https://github.com/openembedded/meta-openembedded.git" \
    "${META_OE_COMMIT}" || {
        return 1 2>/dev/null || exit 1
    }

ensure_repo \
    "${PROJECT_DIR}/meta-raspberrypi" \
    "https://git.yoctoproject.org/meta-raspberrypi" \
    "${META_RPI_COMMIT}" || {
        return 1 2>/dev/null || exit 1
    }

ensure_repo \
    "${PROJECT_DIR}/meta-qt5" \
    "https://github.com/meta-qt5/meta-qt5.git" \
    "${META_QT5_COMMIT}" || {
        return 1 2>/dev/null || exit 1
    }

# ============================================================
# Final commit verification for all repositories
# ============================================================

echo
echo "[INFO] Final pinned-commit verification..."

for repo_info in \
    "${POKY_DIR}:${POKY_COMMIT}" \
    "${PROJECT_DIR}/meta-openembedded:${META_OE_COMMIT}" \
    "${PROJECT_DIR}/meta-raspberrypi:${META_RPI_COMMIT}" \
    "${PROJECT_DIR}/meta-qt5:${META_QT5_COMMIT}"
do
    repo_dir="${repo_info%%:*}"
    expected_commit="${repo_info##*:}"
    actual_commit="$(git -C "${repo_dir}" rev-parse HEAD)" || {
        die "Cannot verify ${repo_dir}."
        return 1 2>/dev/null || exit 1
    }

    actual_branch="$(git -C "${repo_dir}" symbolic-ref --quiet --short HEAD)" || return 1
    if [[ "${actual_commit}" != "${expected_commit}" || "${actual_branch}" != "${BRANCH}" ]]; then
        die "Final verification failed for ${repo_dir}."
        return 1 2>/dev/null || exit 1
    fi

    echo "[OK] $(basename "${repo_dir}"): ${actual_commit}"
done

# ============================================================
# Initialize Yocto build environment
# ============================================================

# Source this script to preserve environment variables in the
# current shell.
cd "${POKY_DIR}" || {
    die "Cannot enter ${POKY_DIR}"
    return 1 2>/dev/null || exit 1
}

source "${POKY_DIR}/oe-init-build-env" \
    "${PROJECT_DIR}/${BUILD_DIR}" || {
        die "Failed to initialize Yocto build environment."
        return 1 2>/dev/null || exit 1
    }

# ============================================================
# Apply project configuration
# ============================================================

echo
echo "[INFO] Applying bblayers.conf..."

bash "${SETUP_BBLAYERS_SCRIPT}" || {
    die "Failed to configure bblayers.conf."
    return 1 2>/dev/null || exit 1
}

echo
echo "[INFO] Applying local.conf..."

bash "${SETUP_BUILD_SCRIPT}" || {
    die "Failed to configure local.conf."
    return 1 2>/dev/null || exit 1
}

# ============================================================
# Complete
# ============================================================

echo
echo "[INFO] Yocto build environment is ready."
echo "Build directory: ${BUILDDIR:-$(pwd)}"
echo "Yocto branch   : ${BRANCH}"
echo
echo "Pinned commits:"
echo "  poky             : ${POKY_COMMIT}"
echo "  meta-openembedded: ${META_OE_COMMIT}"
echo "  meta-raspberrypi : ${META_RPI_COMMIT}"
echo "  meta-qt5        : ${META_QT5_COMMIT}"
echo
echo "Next commands:"
echo "  bitbake-layers show-layers"
echo "  bitbake -p"
echo "  bitbake usb-image"
