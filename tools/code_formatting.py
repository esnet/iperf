import subprocess  # subprocess.run
import argparse
import logging
import sys


def parse_args():
    """
    Parse command line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--format-style',
                        help='Style for formatting code around applied fixes',
                        dest='format_style',
                        type=str,
                        default=".clang-format")
    parser.add_argument('--mode',
                        help='0: Show formatting issues without make the formatting changes.'
                        '1: Inplace edit <file>s.',
                        dest='mode',
                        type=str,
                        default="0")
    return parser


CLANG_FORMAT_INPLACE = "clang-format --Werror -style=file:{format_style} -i {files}"
CLANG_FORMAT_DRY_RUN = "clang-format --Werror -style=file:{format_style} --dry-run {files}"


def print_usage() -> None:
    """
    Print the usage of the script.
    """
    logging.info(
        "Usage: python3 code_formatting.py [options] <source0> <source1> ... <sourceN>")
    logging.info("Options:")
    logging.info(
        "  --format-style  Style for formatting code around applied fixes")
    logging.info(
        "  --mode  0: Show formatting issues without make the formatting changes.' \
         ' 1: Inplace edit <file>s")


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    local_parser = parse_args()
    ns, args = local_parser.parse_known_args()
    if not ns.format_style:
        logging.error("Format style is required")
        print_usage()
        sys.exit(1)
    if not ns.mode:
        logging.error("Mode is required")
        print_usage()
        sys.exit(1)
    if not args:
        logging.error("Source files are required")
        print_usage()
        sys.exit(1)
    # get the source files from args
    INPUT_FILES = ""
    logging.info("Start analyzing the following source files:")
    for file in args:
        # Skip `examples` folder
        if any(substring in file for substring in ["examples/"]):
            continue
        logging.info("%s", file)
        INPUT_FILES += " "
        INPUT_FILES += file
    if not INPUT_FILES:
        logging.error("No source files to analyze")
        sys.exit(1)
    # format input arguments to clang-format command
    CLANG_FORMAT_CMD = ""
    if ns.mode == "1":
        CLANG_FORMAT_CMD = CLANG_FORMAT_INPLACE.format(
            format_style=ns.format_style, files=INPUT_FILES)
    elif ns.mode == "0":
        CLANG_FORMAT_CMD = CLANG_FORMAT_DRY_RUN.format(
            format_style=ns.format_style, files=INPUT_FILES)
    else:
        logging.error("Invalid mode")
        print_usage()
        sys.exit(1)
    logging.info("Executing clang-format command: %s", CLANG_FORMAT_CMD)

    # Split the command into a list as required by subprocess.run()
    command_list = CLANG_FORMAT_CMD.split()

    # Run the command and capture the output
    completed_process = subprocess.run(
        command_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
    # Get the output from the CompletedProcess instance
    console_error = completed_process.stderr
    if not console_error:
        logging.info("Code formatting check passed")
    else:
        logging.info("console_error: %s", console_error)
        logging.error("##############################################")
        logging.error("##         Code formatting check failed     ##")
        logging.error("##############################################")
        sys.exit(1)
