#!/usr/bin/env python3
import argparse
import subprocess
import shutil
import sys
from pathlib import Path

# 定义构建工程目录
ROOT = Path(__file__).parent.resolve()
BUILD_DIR = ROOT / "build"

# 统一执行命令
def run(cmd):
    print(">>", " ".join(cmd))
    subprocess.check_call(cmd)


def clean(args):
    # ---- clean ----
    if args.clean and BUILD_DIR.exists():
        print("[CLEAN] removing build directory")
        shutil.rmtree(BUILD_DIR)

def create_configure_parser():
    parser = argparse.ArgumentParser(
        description="C/C++ project build helper"
    )
    # 构建类型
    parser.add_argument(
        "--type",
        default="Debug",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="Build type"
    )
    # 指定C编译器
    parser.add_argument(
        "--cc",
        help="C compiler (e.g. gcc, clang)"
    )
    # 指定Cpp编译器
    parser.add_argument(
        "--cxx",
        help="C++ compiler (e.g. g++, clang++)"
    )
    # 交叉编译
    parser.add_argument(
        "--toolchain",
        help="CMake toolchain file"
    )
    # 删除 build 目录
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove build directory before building"
    )
    # 构建后跑测试
    parser.add_argument(
        "--test",
        action="store_true",
        help="Run unit tests after build"
    )
    return parser

def main():
    # 创建和配置参数解析器
    parser = create_configure_parser()
    args = parser.parse_args()
    raw_args = sys.argv[1:]

    # ---- 阶段开关 ----
    do_clean = False
    do_configure = False
    do_build = False
    do_test = False
 
    # ---- clean ----
    if args.clean and BUILD_DIR.exists():
        do_clean = True
    # ---- clean only ----
    if raw_args == ["--clean"]:
        print("[MODE] clean only")
    else:
        # 非 clean-only，才进入构建流程
        do_configure = True
        do_build = True
        if args.test:
            do_test = True 

    # ---- clean ----
    if do_clean:
        clean(args)

    # ---- cmake configure ----
    if do_configure:
        cmake_cmd = [
            "cmake",
            "-S", str(ROOT),
            "-B", str(BUILD_DIR),
            f"-DCMAKE_BUILD_TYPE={args.type}",
        ]

        if args.cc:
            cmake_cmd.append(f"-DCMAKE_C_COMPILER={args.cc}")

        if args.cxx:
            cmake_cmd.append(f"-DCMAKE_CXX_COMPILER={args.cxx}")

        if args.toolchain:
            cmake_cmd.append(f"-DCMAKE_TOOLCHAIN_FILE={args.toolchain}")

        run(cmake_cmd)

    # ---- build ----
    if do_build:
        run([
            "cmake",
            "--build", str(BUILD_DIR),
            "-j"
        ])

    # ---- test ----
    if do_test:
        run([
            "ctest",
            "--test-dir", str(BUILD_DIR),
            "--output-on-failure",
            "-L libqos"
        ])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] command failed: {e}")
        sys.exit(1)
