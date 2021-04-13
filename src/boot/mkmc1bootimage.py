#!/usr/bin/env python3
# -*- mode: python; tab-width: 4; indent-tabs-mode: nil; -*-
# --------------------------------------------------------------------------------------------------
# Copyright (c) 2021 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied warranty. In no event will the
# authors be held liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose, including commercial
# applications, and to alter it and redistribute it freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not claim that you wrote
#     the original software. If you use this software in a product, an acknowledgment in the
#     product documentation would be appreciated but is not required.
#
#  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
#     being the original software.
#
#  3. This notice may not be removed or altered from any source distribution.
# --------------------------------------------------------------------------------------------------

import argparse
import crc32c
import struct

_MAX_CODE_SIZE = 512 - 8


def convert(boot, img, extras):
    # Read the raw boot data.
    with open(boot, "rb") as f:
        data = f.read()
    code_size = len(data)

    # Pad the data to _MAX_CODE_SIZE bytes.
    if code_size > _MAX_CODE_SIZE:
        print(f"Code size is too large: {len(data)} (max {_MAX_CODE_SIZE})")
    pad = _MAX_CODE_SIZE - code_size
    if pad > 0:
        data += bytearray(pad)

    # Prepend the header.
    magic = 0x4231434D
    crc = crc32c.crc32c(data)
    data = struct.pack("<LL", magic, crc) + data

    print("--------------------------------------------------------------------")
    print(f"Code size: {code_size} bytes (padded to {len(data)} bytes)")
    print(f"Magic ID:  {magic:#08x}")
    print(f"CRC32C:    {crc:#08x}")
    print("--------------------------------------------------------------------")

    # Append optional extra files.
    block_no = 1
    for extra in extras:
        with open(extra, "rb") as f:
            extra_data = f.read()
        extra_size = len(extra_data)
        num_blocks = (extra_size + 511) >> 9
        pad = 512 * num_blocks - extra_size
        if pad > 0:
            extra_data += bytearray(pad)
        data += extra_data

        print(f"File:   {extra}")
        print(f"Block:  {block_no}, Num. blocks: {num_blocks}")
        print("--------------------------------------------------------------------")

        block_no += num_blocks

    # Write the boot image.
    with open(img, "wb") as f:
        f.write(data)

    print(f"\nBoot image written to {img}")


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
        description="Convert raw files to an MC1 boot image"
    )
    parser.add_argument("boot", metavar="BOOT_CODE", help="the raw boot code (max 504 bytes)")
    parser.add_argument("img", metavar="IMAGE", help="the boot image")
    parser.add_argument("extras", nargs="*", metavar="EXTRA", help="extra file(s)")
    args = parser.parse_args()

    # Convert the file.
    convert(args.boot, args.img, args.extras)


if __name__ == "__main__":
    main()