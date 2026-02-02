import os
import json
import argparse

DEBUG = True  # set False to disable debug output


def debug(message):
    if DEBUG:
        print(f"DEBUG: {message}")


USER_DIR = os.path.expanduser('~')

# C:\Users\{USER}\AppData\Local\Arduino15

ARDUINO_DEFAULT_DIR = os.path.join(USER_DIR, 'AppData', 'Local', 'Arduino15')

# C:\Users\{USER}\Documents\Arduino\libraries

ONE_DRIVE_DIR = os.path.join(USER_DIR, 'OneDrive')


ARDUINO_LIB_DIR = ""
ESP32_CORE_DIR = ""
ESP8266_CORE_DIR = ""
AVR_GCC_DIR = ""
# ARDUINO_HEADER_FILE_PATH = ""
BOARD_VERSION = ""
BOARD_CORE_PATH = ""

def remove_tmp(fileName):
    if os.path.exists(fileName):
        os.remove(fileName)

if os.path.exists(ARDUINO_DEFAULT_DIR):
    ESP32_CORE_DIR = os.path.join(ARDUINO_DEFAULT_DIR, 'packages', 'esp32')
    ESP8266_CORE_DIR = os.path.join(ARDUINO_DEFAULT_DIR, 'packages', 'esp8266')
    AVR_GCC_DIR = os.path.join(ARDUINO_DEFAULT_DIR, 'packages', 'arduino', 'tools', 'avr-gcc')
    if not os.path.exists(ESP32_CORE_DIR):
        ESP32_CORE_DIR = ""
    if not os.path.exists(ESP32_CORE_DIR):
        ESP8266_CORE_DIR = ""
    if os.path.exists(AVR_GCC_DIR):
        AVR_GCC_DIR = os.path.join(AVR_GCC_DIR, os.listdir(AVR_GCC_DIR)[0], 'avr', 'include', 'avr')
    else:
        AVR_GCC_DIR = ""



if os.path.exists(os.path.join(ONE_DRIVE_DIR, 'Documents')):
    ARDUINO_LIB_DIR = os.path.join(ONE_DRIVE_DIR, 'Documents', 'Arduino', 'libraries')
else:
    ARDUINO_LIB_DIR = os.path.join(USER_DIR, 'Documents', 'Arduino', 'libraries')


def list_files_and_folders(path):
    if len(path) > 0:
        with open("FilesNFolders.txt", "a") as file:
            for entry in os.scandir(path):
                if entry.is_dir():
                    file.write(f"{entry.path}\n")
                    # debug(f"{entry.path}")
                    if "esp32" in path or  "esp8266" in path:
                        list_files_and_folders(entry.path)


def sensitize_dir(file_name,ignore_board_dir) -> list:
    include_path = list()
    keywords_to_remove = {'test','tests','example', 'examples', 'git', 'tmp', 'avr','extras','samd','AmebaD','rp2040'}
    with open(file_name, 'r') as file:
        directories = file.readlines()
    
    # print(f"{'7.3.0-atmel3.6.1-arduino7\avr\include\avr' in directories}")

    unique_directories = [directory.strip() for directory in directories
                          if not any(keyword in directory.lower() for keyword in keywords_to_remove)]

    include_path.append("${workspaceFolder}/**")

    # with open(file_name, 'w') as file:
    for directory in unique_directories:
        # if 'avr-gcc' in directory:
            # print(f"directory : {directory}")
        if not ignore_board_dir in directory:
            include_path.append(f"{directory}\\**")

    return include_path


def board_version(board):

    global BOARD_VERSION,BOARD_CORE_PATH
    
    if board in ESP32_CORE_DIR:
        target_board = ESP32_CORE_DIR
    else:
        target_board = ESP8266_CORE_DIR

    try:
        BOARD_CORE_PATH = os.path.join(target_board, 'hardware', board)
        BOARD_VERSION = os.listdir(BOARD_CORE_PATH)[0]
    except Exception as e:
        print("Error : ", e)

def arduino_header_file(board) -> list :
    # C:\\Users\\{USER}\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\

    # global BOARD_VERSION

    # if "esp32" in target_board:
    #     # C:\Users\{USER}\AppData\Local\Arduino15\packages\esp32\hardware\esp32\{board_version}\cores\esp32
    #     board_core_path = os.path.join(target_board, 'hardware', board)
    #     BOARD_VERSION = os.listdir(board_core_path)[0]
    #     arduino_header_file_path = os.path.join(board_core_path, BOARD_VERSION, 'cores', board,'Arduino.h')
    # else:
    #     # C:\Users\{USER}\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\{board_version}\cores\esp8266
    #     board_core_path = os.path.join(target_board, 'hardware', board)
    #     BOARD_VERSION = os.listdir(board_core_path)[0]
    arduino_header_file_path = os.path.join(BOARD_CORE_PATH, BOARD_VERSION, 'cores', board,'Arduino.h')

    return [arduino_header_file_path]

def get_gcc_path(board) -> str:
    # "C:\\Users\\{USER}\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\xtensa-esp32-elf-gcc\\gcc8_4_0-esp-2021r2-patch5\\bin\\xtensa-esp32-elf-g++",
    if board in ESP32_CORE_DIR:
        target_board = ESP32_CORE_DIR
    else:
        target_board = ESP8266_CORE_DIR

    tools_dir = os.path.join(target_board, 'tools')
    xtensa_gcc_dir = os.path.join(tools_dir,[tool for tool in os.listdir(tools_dir) if 'xtensa' in tool][0])
    xtensa_version_dir = os.path.join(xtensa_gcc_dir,os.listdir(xtensa_gcc_dir)[0])
    xtensa_bin_dir = os.path.join(xtensa_version_dir, 'bin')
    gcc_bin_dir_list = os.listdir(xtensa_bin_dir)
    gcc_path = os.path.join(xtensa_bin_dir,[glist for glist in gcc_bin_dir_list if '-elf-gcc.exe' in glist][0])
    return gcc_path

def create_json(output_file, board):
    ignore_board = ""
    if board in "esp32":
        ignore_board = ESP8266_CORE_DIR
    elif board in "esp8266":
        ignore_board = ESP32_CORE_DIR

    board_version(board)

    remove_tmp("FilesNFolders.txt")

    list_files_and_folders(ARDUINO_LIB_DIR)
    # list_files_and_folders(ARDUINO_DEFAULT_DIR)

    if not 'esp8266' in  ignore_board:
        list_files_and_folders(os.path.join(ESP8266_CORE_DIR,'hardware',board,BOARD_VERSION))
    if not 'esp32' in ignore_board:
        list_files_and_folders(os.path.join(ESP32_CORE_DIR,'hardware',board,BOARD_VERSION))
    # list_files_and_folders(AVR_GCC_DIR)

    compiler_args = {"-mlongcalls",
                    "-Wno-frame-address",
                    "-ffunction-sections",
                    "-fdata-sections",
                    "-Wno-error=unused-function",
                    "-Wno-error=unused-variable",
                    "-Wno-error=deprecated-declarations",
                    "-Wno-unused-parameter",
                    "-Wno-sign-compare",
                    "-freorder-blocks",
                    "-Wwrite-strings",
                    "-fstack-protector",
                    "-fstrict-volatile-bitfields",
                    "-Wno-error=unused-but-set-variable",
                    "-fno-jump-tables",
                    "-fno-tree-switch-conversion",
                    "-std=gnu++11",
                    "-fexceptions",
                    "-fno-rtti"}

    defines = {
                "HAVE_CONFIG_H",
                "MBEDTLS_CONFIG_FILE=\"mbedtls/esp_config.h\"",
                "UNITY_INCLUDE_CONFIG_H",
                "WITH_POSIX",
                "_GNU_SOURCE",
                "IDF_VER=\"v4.4.3\"",
                "ESP_PLATFORM",
                "_POSIX_READER_WRITER_LOCKS",
                "F_CPU=240000000L",
                "ARDUINO=10607",
                "ARDUINO_ESP32_DEV",
                "ARDUINO_ARCH_ESP32",
                "ARDUINO_BOARD=\"ESP32_DEV\"",
                "ARDUINO_VARIANT=\"esp32\"",
                "ARDUINO_PARTITION_default",
                "ESP32",
                "CORE_DEBUG_LEVEL=0",
                "ARDUINO_RUNNING_CORE=1",
                "ARDUINO_EVENT_RUNNING_CORE=1",
                "ARDUINO_USB_CDC_ON_BOOT=0",
                "__DBL_MIN_EXP__=(-1021)",
                "__FLT32X_MAX_EXP__=1024",
                "__cpp_attributes=200809",
                "__UINT_LEAST16_MAX__=0xffff",
                "__ATOMIC_ACQUIRE=2",
                "__FLT_MIN__=1.1754943508222875e-38F",
                "__GCC_IEC_559_COMPLEX=0",
                "__cpp_aggregate_nsdmi=201304",
                "__UINT_LEAST8_TYPE__=unsigned char",
                "__INTMAX_C(c)=c ## LL",
                "__CHAR_BIT__=8",
                "__UINT8_MAX__=0xff",
                "__WINT_MAX__=0xffffffffU",
                "__FLT32_MIN_EXP__=(-125)",
                "__cpp_static_assert=200410",
                "__ORDER_LITTLE_ENDIAN__=1234",
                "__SIZE_MAX__=0xffffffffU",
                "__WCHAR_MAX__=0xffff",
                "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1=1",
                "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2=1",
                "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4=1",
                "__DBL_DENORM_MIN__=double(4.9406564584124654e-324L)",
                "__GCC_ATOMIC_CHAR_LOCK_FREE=2",
                "__GCC_IEC_559=0",
                "__FLT32X_DECIMAL_DIG__=17",
                "__FLT_EVAL_METHOD__=0",
                "__cpp_binary_literals=201304",
                "__FLT64_DECIMAL_DIG__=17",
                "__GCC_ATOMIC_CHAR32_T_LOCK_FREE=2",
                "__cpp_variadic_templates=200704",
                "__UINT_FAST64_MAX__=0xffffffffffffffffULL",
                "__SIG_ATOMIC_TYPE__=int",
                "__DBL_MIN_10_EXP__=(-307)",
                "__FINITE_MATH_ONLY__=0",
                "__cpp_variable_templates=201304",
                "__GNUC_PATCHLEVEL__=0",
                "__FLT32_HAS_DENORM__=1",
                "__UINT_FAST8_MAX__=0xffffffffU",
                "__has_include(STR)=__has_include__(STR)",
                "__DEC64_MAX_EXP__=385",
                "__INT8_C(c)=c",
                "__INT_LEAST8_WIDTH__=8",
                "__UINT_LEAST64_MAX__=0xffffffffffffffffULL",
                "__SHRT_MAX__=0x7fff",
                "__LDBL_MAX__=1.7976931348623157e+308L",
                "__UINT_LEAST8_MAX__=0xff",
                "__GCC_ATOMIC_BOOL_LOCK_FREE=2",
                "__UINTMAX_TYPE__=long long unsigned int",
                "__DEC32_EPSILON__=1E-6DF",
                "__FLT_EVAL_METHOD_TS_18661_3__=0",
                "__CHAR_UNSIGNED__=1",
                "__UINT32_MAX__=0xffffffffU",
                "__GXX_EXPERIMENTAL_CXX0X__=1",
                "__LDBL_MAX_EXP__=1024",
                "__WINT_MIN__=0U",
                "__INT_LEAST16_WIDTH__=16",
                "__SCHAR_MAX__=0x7f",
                "__WCHAR_MIN__=0",
                "__INT64_C(c)=c ## LL",
                "__DBL_DIG__=15",
                "__GCC_ATOMIC_POINTER_LOCK_FREE=2",
                "__SIZEOF_INT__=4",
                "__SIZEOF_POINTER__=4",
                "__GCC_ATOMIC_CHAR16_T_LOCK_FREE=2",
                "__USER_LABEL_PREFIX__",
                "__STDC_HOSTED__=1",
                "__LDBL_HAS_INFINITY__=1",
                "__XTENSA_EL__=1",
                "__FLT32_DIG__=6",
                "__FLT_EPSILON__=1.1920928955078125e-7F",
                "__GXX_WEAK__=1",
                "__SHRT_WIDTH__=16",
                "__LDBL_MIN__=2.2250738585072014e-308L",
                "__DEC32_MAX__=9.999999E96DF",
                "__cpp_threadsafe_static_init=200806",
                "__FLT32X_HAS_INFINITY__=1",
                "__INT32_MAX__=0x7fffffff",
                "__INT_WIDTH__=32",
                "__SIZEOF_LONG__=4",
                "__UINT16_C(c)=c",
                "__PTRDIFF_WIDTH__=32",
                "__DECIMAL_DIG__=17",
                "__FLT64_EPSILON__=2.2204460492503131e-16F64",
                "__INTMAX_WIDTH__=64",
                "__FLT64_MIN_EXP__=(-1021)",
                "__has_include_next(STR)=__has_include_next__(STR)",
                "__LDBL_HAS_QUIET_NAN__=1",
                "__FLT64_MANT_DIG__=53",
                "__GNUC__=8",
                "__GXX_RTTI=1",
                "__cpp_delegating_constructors=200604",
                "__FLT_HAS_DENORM__=1",
                "__SIZEOF_LONG_DOUBLE__=8",
                "__BIGGEST_ALIGNMENT__=16",
                "__STDC_UTF_16__=1",
                "__FLT64_MAX_10_EXP__=308",
                "__FLT32_HAS_INFINITY__=1",
                "__DBL_MAX__=double(1.7976931348623157e+308L)",
                "__cpp_raw_strings=200710",
                "__INT_FAST32_MAX__=0x7fffffff",
                "__DBL_HAS_INFINITY__=1",
                "__DEC32_MIN_EXP__=(-94)",
                "__INTPTR_WIDTH__=32",
                "__FLT32X_HAS_DENORM__=1",
                "__INT_FAST16_TYPE__=int",
                "__LDBL_HAS_DENORM__=1",
                "__cplusplus=201402L",
                "__cpp_ref_qualifiers=200710",
                "__DEC128_MAX__=9.999999999999999999999999999999999E6144DL",
                "__INT_LEAST32_MAX__=0x7fffffff",
                "__DEC32_MIN__=1E-95DF",
                "__DEPRECATED=1",
                "__cpp_rvalue_references=200610",
                "__DBL_MAX_EXP__=1024",
                "__WCHAR_WIDTH__=16",
                "__FLT32_MAX__=3.4028234663852886e+38F32",
                "__DEC128_EPSILON__=1E-33DL",
                "__PTRDIFF_MAX__=0x7fffffff",
                "__FLT32_HAS_QUIET_NAN__=1",
                "__GNUG__=8",
                "__LONG_LONG_MAX__=0x7fffffffffffffffLL",
                "__SIZEOF_SIZE_T__=4",
                "__cpp_rvalue_reference=200610",
                "__cpp_nsdmi=200809",
                "__SIZEOF_WINT_T__=4",
                "__LONG_LONG_WIDTH__=64",
                "__cpp_initializer_lists=200806",
                "__FLT32_MAX_EXP__=128",
                "__cpp_hex_float=201603",
                "__GXX_ABI_VERSION=1013",
                "__FLT_MIN_EXP__=(-125)",
                "__cpp_lambdas=200907",
                "__INT_FAST64_TYPE__=long long int",
                "__FP_FAST_FMAF=1",
                "__FLT64_DENORM_MIN__=4.9406564584124654e-324F64",
                "__DBL_MIN__=double(2.2250738585072014e-308L)",
                "__FLT32X_EPSILON__=2.2204460492503131e-16F32x",
                "__FLT64_MIN_10_EXP__=(-307)",
                "__DEC128_MIN__=1E-6143DL",
                "__REGISTER_PREFIX__",
                "__UINT16_MAX__=0xffff",
                "__DBL_HAS_DENORM__=1",
                "__FLT32_MIN__=1.1754943508222875e-38F32",
                "__UINT8_TYPE__=unsigned char",
                "__NO_INLINE__=1",
                "__FLT_MANT_DIG__=24",
                "__LDBL_DECIMAL_DIG__=17",
                "__VERSION__=\"8.4.0\"",
                "__UINT64_C(c)=c ## ULL",
                "__cpp_unicode_characters=200704",
                "__cpp_decltype_auto=201304",
                "__GCC_ATOMIC_INT_LOCK_FREE=2",
                "__FLT32_MANT_DIG__=24",
                "__FLOAT_WORD_ORDER__=__ORDER_LITTLE_ENDIAN__",
                "__SCHAR_WIDTH__=8",
                "__INT32_C(c)=c",
                "__DEC64_EPSILON__=1E-15DD",
                "__ORDER_PDP_ENDIAN__=3412",
                "__DEC128_MIN_EXP__=(-6142)",
                "__FLT32_MAX_10_EXP__=38",
                "__INT_FAST32_TYPE__=int",
                "__UINT_LEAST16_TYPE__=short unsigned int",
                "__INT16_MAX__=0x7fff",
                "__cpp_rtti=199711",
                "__SIZE_TYPE__=unsigned int",
                "__UINT64_MAX__=0xffffffffffffffffULL",
                "__INT8_TYPE__=signed char",
                "__cpp_digit_separators=201309",
                "__ELF__=1",
                "__xtensa__=1",
                "__FLT_RADIX__=2",
                "__INT_LEAST16_TYPE__=short int",
                "__LDBL_EPSILON__=2.2204460492503131e-16L",
                "__UINTMAX_C(c)=c ## ULL",
                "__SIG_ATOMIC_MAX__=0x7fffffff",
                "__GCC_ATOMIC_WCHAR_T_LOCK_FREE=2",
                "__SIZEOF_PTRDIFF_T__=4",
                "__FLT32X_MANT_DIG__=53",
                "__FLT32X_MIN_EXP__=(-1021)",
                "__DEC32_SUBNORMAL_MIN__=0.000001E-95DF",
                "__INT_FAST16_MAX__=0x7fffffff",
                "__FLT64_DIG__=15",
                "__UINT_FAST32_MAX__=0xffffffffU",
                "__UINT_LEAST64_TYPE__=long long unsigned int",
                "__FLT_HAS_QUIET_NAN__=1",
                "__FLT_MAX_10_EXP__=38",
                "__LONG_MAX__=0x7fffffffL",
                "__DEC128_SUBNORMAL_MIN__=0.000000000000000000000000000000001E-6143DL",
                "__FLT_HAS_INFINITY__=1",
                "__cpp_unicode_literals=200710",
                "__UINT_FAST16_TYPE__=unsigned int",
                "__DEC64_MAX__=9.999999999999999E384DD",
                "__INT_FAST32_WIDTH__=32",
                "__CHAR16_TYPE__=short unsigned int",
                "__PRAGMA_REDEFINE_EXTNAME=1",
                "__SIZE_WIDTH__=32",
                "__INT_LEAST16_MAX__=0x7fff",
                "__DEC64_MANT_DIG__=16",
                "__INT64_MAX__=0x7fffffffffffffffLL",
                "__UINT_LEAST32_MAX__=0xffffffffU",
                "__FLT32_DENORM_MIN__=1.4012984643248171e-45F32",
                "__GCC_ATOMIC_LONG_LOCK_FREE=2",
                "__SIG_ATOMIC_WIDTH__=32",
                "__INT_LEAST64_TYPE__=long long int",
                "__INT16_TYPE__=short int",
                "__INT_LEAST8_TYPE__=signed char",
                "__DEC32_MAX_EXP__=97",
                "__INT_FAST8_MAX__=0x7fffffff",
                "__INTPTR_MAX__=0x7fffffff",
                "__cpp_sized_deallocation=201309",
                "__cpp_range_based_for=200907",
                "__FLT64_HAS_QUIET_NAN__=1",
                "__FLT32_MIN_10_EXP__=(-37)",
                "__EXCEPTIONS=1",
                "__LDBL_MANT_DIG__=53",
                "__DBL_HAS_QUIET_NAN__=1",
                "__FLT64_HAS_INFINITY__=1",
                "__SIG_ATOMIC_MIN__=(-__SIG_ATOMIC_MAX__ - 1)",
                "__cpp_return_type_deduction=201304",
                "__INTPTR_TYPE__=int",
                "__UINT16_TYPE__=short unsigned int",
                "__WCHAR_TYPE__=short unsigned int",
                "__SIZEOF_FLOAT__=4",
                "__UINTPTR_MAX__=0xffffffffU",
                "__INT_FAST64_WIDTH__=64",
                "__DEC64_MIN_EXP__=(-382)",
                "__cpp_decltype=200707",
                "__FLT32_DECIMAL_DIG__=9",
                "__INT_FAST64_MAX__=0x7fffffffffffffffLL",
                "__GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1",
                "__FLT_DIG__=6",
                "__UINT_FAST64_TYPE__=long long unsigned int",
                "__INT_MAX__=0x7fffffff",
                "__INT64_TYPE__=long long int",
                "__FLT_MAX_EXP__=128",
                "__DBL_MANT_DIG__=53",
                "__cpp_inheriting_constructors=201511",
                "__INT_LEAST64_MAX__=0x7fffffffffffffffLL",
                "__FP_FAST_FMAF32=1",
                "__DEC64_MIN__=1E-383DD",
                "__WINT_TYPE__=unsigned int",
                "__UINT_LEAST32_TYPE__=unsigned int",
                "__SIZEOF_SHORT__=2",
                "__LDBL_MIN_EXP__=(-1021)",
                "__FLT64_MAX__=1.7976931348623157e+308F64",
                "__WINT_WIDTH__=32",
                "__INT_LEAST8_MAX__=0x7f",
                "__FLT32X_MAX_10_EXP__=308",
                "__WCHAR_UNSIGNED__=1",
                "__LDBL_MAX_10_EXP__=308",
                "__ATOMIC_RELAXED=0",
                "__DBL_EPSILON__=double(2.2204460492503131e-16L)",
                "__XTENSA_WINDOWED_ABI__=1",
                "__UINT8_C(c)=c",
                "__FLT64_MAX_EXP__=1024",
                "__INT_LEAST32_TYPE__=int",
                "__SIZEOF_WCHAR_T__=2",
                "__INT_FAST8_TYPE__=int",
                "__GNUC_STDC_INLINE__=1",
                "__FLT64_HAS_DENORM__=1",
                "__FLT32_EPSILON__=1.1920928955078125e-7F32",
                "__DBL_DECIMAL_DIG__=17",
                "__STDC_UTF_32__=1",
                "__INT_FAST8_WIDTH__=32",
                "__DEC_EVAL_METHOD__=2",
                "__FLT32X_MAX__=1.7976931348623157e+308F32x",
                "__XTENSA__=1",
                "__ORDER_BIG_ENDIAN__=4321",
                "__cpp_runtime_arrays=198712",
                "__UINT64_TYPE__=long long unsigned int",
                "__UINT32_C(c)=c ## U",
                "__INTMAX_MAX__=0x7fffffffffffffffLL",
                "__cpp_alias_templates=200704",
                "__BYTE_ORDER__=__ORDER_LITTLE_ENDIAN__",
                "__FLT_DENORM_MIN__=1.4012984643248171e-45F",
                "__INT8_MAX__=0x7f",
                "__LONG_WIDTH__=32",
                "__UINT_FAST32_TYPE__=unsigned int",
                "__CHAR32_TYPE__=unsigned int",
                "__FLT_MAX__=3.4028234663852886e+38F",
                "__cpp_constexpr=201304",
                "__INT32_TYPE__=int",
                "__SIZEOF_DOUBLE__=8",
                "__cpp_exceptions=199711",
                "__FLT_MIN_10_EXP__=(-37)",
                "__FLT64_MIN__=2.2250738585072014e-308F64",
                "__INT_LEAST32_WIDTH__=32",
                "__INTMAX_TYPE__=long long int",
                "__DEC128_MAX_EXP__=6145",
                "__FLT32X_HAS_QUIET_NAN__=1",
                "__ATOMIC_CONSUME=1",
                "__GNUC_MINOR__=4",
                "__INT_FAST16_WIDTH__=32",
                "__UINTMAX_MAX__=0xffffffffffffffffULL",
                "__DEC32_MANT_DIG__=7",
                "__FLT32X_DENORM_MIN__=4.9406564584124654e-324F32x",
                "__DBL_MAX_10_EXP__=308",
                "__LDBL_DENORM_MIN__=4.9406564584124654e-324L",
                "__INT16_C(c)=c",
                "__cpp_generic_lambdas=201304",
                "__STDC__=1",
                "__FLT32X_DIG__=15",
                "__PTRDIFF_TYPE__=int",
                "__ATOMIC_SEQ_CST=5",
                "__UINT32_TYPE__=unsigned int",
                "__FLT32X_MIN_10_EXP__=(-307)",
                "__UINTPTR_TYPE__=unsigned int",
                "__DEC64_SUBNORMAL_MIN__=0.000000000000001E-383DD",
                "__DEC128_MANT_DIG__=34",
                "__LDBL_MIN_10_EXP__=(-307)",
                "__SIZEOF_LONG_LONG__=8",
                "__cpp_user_defined_literals=200809",
                "__GCC_ATOMIC_LLONG_LOCK_FREE=1",
                "__FLT32X_MIN__=2.2250738585072014e-308F32x",
                "__LDBL_DIG__=15",
                "__FLT_DECIMAL_DIG__=9",
                "__UINT_FAST16_MAX__=0xffffffffU",
                "__GCC_ATOMIC_SHORT_LOCK_FREE=2",
                "__INT_LEAST64_WIDTH__=64",
                "__UINT_FAST8_TYPE__=unsigned int",
                "__cpp_init_captures=201304",
                "__ATOMIC_ACQ_REL=4",
                "__ATOMIC_RELEASE=3",
                "USBCON"
    }

    configurations = {
                "name": "Arduino",
                "compilerPath": get_gcc_path(board),
                "compilerArgs": list(compiler_args),
                "intelliSenseMode": "gcc-x86",
                "includePath": sensitize_dir("FilesNFolders.txt", ignore_board),
                "forcedInclude": arduino_header_file(board),
                "cStandard": "gnu11",
                "cppStandard": "gnu++11",
                "defines": list(defines)
                }

    data = {
        "version": 4,
        "configurations": [configurations]
    }

    json_data = json.dumps(data, indent=4)
    with open(output_file, 'w') as f:
        f.write(json_data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("board",help="Which Board You want to make config for ? ")
    BOARD = parser.parse_args().board
    BOARD = BOARD.lower()
    ### Only For Testing ###
    # list_files_and_folders(ARDUINO_LIB_DIR)
    # list_files_and_folders(ARDUINO_DEFAULT_DIR)
    # print(get_gcc_path(BOARD))
    # sensitize_dir("FilesNFolders.txt", "esp8266")
    ### DISBALE THEM ###
    if not os.path.exists("c_cpp_properties.json"):
        if BOARD not in ["esp32", "esp8266"]:
            print("Invalid Input")
            exit()
        create_json("c_cpp_properties.json",BOARD)
        remove_tmp("FilesNFolders.txt")

    else:
        print("All Well")


