#!/usr/bin/env python3

import subprocess

def make():
    build_command = "gcc -Wall passthrough.c `pkg-config fuse3 --cflags --libs` -o passthrough"
    subprocess.run(build_command, shell=True)

if __name__ == "__main__":
    make()