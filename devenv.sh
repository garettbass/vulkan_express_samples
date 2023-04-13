# USAGE: source devenv.sh
#-------------------------------------------------------------------------------

function realpath {
    local path="${1//\\//}"
    if [ "$path" == "." ]; then
        echo "$(pwd)"
    elif [ "$path" == ".." ]; then
        echo "$(dirname "$(pwd)")"
    else
        echo "$(cd "$(dirname "$path")"; pwd)/$(basename "$path")"
    fi
}

#-------------------------------------------------------------------------------

DEVENV_DIR="$(realpath $(dirname "$_"))"
DEVENV_BIN_DIR="$(realpath "$DEVENV_DIR/bin")"
DEVENV_LIB_DIR="$(realpath "$DEVENV_DIR/lib")"

case $(uname | tr '[:upper:]' '[:lower:]') in
    darwin*)
        DEVENV_OS=macos
    ;;
    linux*)
        DEVENV_OS=linux
    ;;
    msys*|mingw*)
        DEVENV_OS=windows
    ;;
    *)
        echo "unsupported operating system"
        exit 1
    ;;
esac

DEVENV_OS_BIN_DIR="$DEVENV_BIN_DIR/$DEVENV_OS"
DEVENV_OS_LIB_DIR="$DEVENV_LIB_DIR/$DEVENV_OS"

#-------------------------------------------------------------------------------

echo ""
echo "DEVENV_DIR:        "$DEVENV_DIR
echo "DEVENV_BIN_DIR:    "$DEVENV_BIN_DIR
echo "DEVENV_LIB_DIR:    "$DEVENV_LIB_DIR
echo ""
echo "DEVENV_OS:         "$DEVENV_OS
echo "DEVENV_OS_BIN_DIR: "$DEVENV_OS_BIN_DIR
echo "DEVENV_OS_LIB_DIR: "$DEVENV_OS_LIB_DIR
echo ""

#-------------------------------------------------------------------------------

echo "compiling cxe for $DEVENV_OS..."
sh "$(realpath "ext/cxe/src/cxe.cpp")"

CXE_DIR="$(realpath "ext/cxe/bin/$DEVENV_OS")"
VULKAN_BIN="$(realpath "$VULKAN_SDK/Bin")"

export CC="${CC:=$(command -v clang)}"
export CXX="${CC:=$(command -v clang)}"

[[ ":$PATH:" =~ ":$CXE_DIR:"    ]] || PATH="$CXE_DIR:$PATH"
[[ ":$PATH:" =~ ":$VULKAN_BIN:" ]] || PATH="$VULKAN_BIN:$PATH"

echo ""
echo "cxe:   " $(which cxe)
echo "dxc:   " $(which dxc)
echo "glslc: " $(which glslc)
