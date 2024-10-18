. "$MICROFRAMEWORK_ROOT"/bin/utils.sh

FIRMWARE_NAME=no-name
FIRMWARE_MCU=unknown
FIRMWARE_TYPE=generic
BUILD_VARIANT=default
BUILD_PROFILE=unknown
HARDWARE_TYPE=unknown

MAJOR_VERSION=0
MINOR_VERSION=0
PATCHLEVEL=0

__GIT_VERSION_PREFIX__=''

while is_opt "$1"; do
    opt="$1"
    value="$2"
    shift 2
    case "$opt" in
    -n) FIRMWARE_NAME="$value" ;;
    -m) FIRMWARE_MCU="$value"  ;;
    -t) FIRMWARE_TYPE="$value" ;;
    -b) BUILD_VARIANT="$value" ;;
    -p) BUILD_PROFILE="$value" ;;
    -h) HARDWARE_TYPE="$value" ;;
    -P) __GIT_VERSION_PREFIX__="$value" ;;
    *) printf 'Unknown option: %s\n' "$opt"
       exit 1
       ;;
    esac
done
