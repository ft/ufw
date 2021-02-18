FIRMWARE_NAME=no-name
FIRMWARE_MCU=unknown
FIRMWARE_TYPE=generic
BUILD_VARIANT=default
BUILD_PROFILE=unknown
HARDWARE_TYPE=unknown

MAJOR_VERSION=0
MINOR_VERSION=0
PATCHLEVEL=0

is_opt () {
    case "$1" in
    --) return 1 ;;
    -*) return 0 ;;
    *) return 1 ;;
    esac
}

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
    *) printf 'Unknown option: %s\n' "$opt"
       exit 1
       ;;
    esac
done
