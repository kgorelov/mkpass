#!/bin/env python3

import os
import argparse

def generate_cpp_files(input_file, name, output_dir="."):
    with open(input_file, 'r') as f:
        words = [line.strip() for line in f if line.strip()]

    blob = ""
    offsets = []
    current_offset = 0

    for word in words:
        offsets.append(current_offset)
        blob += word + "\0"
        current_offset += len(word) + 1

    header_file = os.path.join(output_dir, f"{name}.h")
    source_file = os.path.join(output_dir, f"{name}.cpp")

    guard = f"{name.upper()}_H"
    get_word_func = f"{name}_get_word"
    get_word_count_func = f"{name}_get_word_count"

    # Header file
    with open(header_file, 'w') as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write("const char* " + get_word_func + "(int idx);\n")
        f.write("int " + get_word_count_func + "();\n\n")
        f.write(f"#endif // {guard}\n")

    # Source file
    with open(source_file, 'w') as f:
        f.write(f"#include \"{os.path.basename(header_file)}\"\n\n")
        
        # Write blob as hex bytes to avoid escaping issues and compiler limits on string literal length
        f.write("static const char words_blob[] = {\n")
        hex_blob = []
        for char in blob:
            hex_blob.append(hex(ord(char)))
        
        # Write in chunks for readability
        for i in range(0, len(hex_blob), 16):
            f.write("    " + ", ".join(hex_blob[i:i+16]) + ",\n")
        f.write("};\n\n")

        # Write offsets
        f.write("static const int word_offsets[] = {\n")
        for i in range(0, len(offsets), 16):
            f.write("    " + ", ".join(map(str, offsets[i:i+16])) + ",\n")
        f.write("};\n\n")

        f.write("static const int word_count = " + str(len(words)) + ";\n\n")

        f.write("const char* " + get_word_func + "(int idx) {\n")
        f.write("    if (idx < 0 || idx >= word_count) return nullptr;\n")
        f.write("    return words_blob + word_offsets[idx];\n")
        f.write("}\n\n")
        
        f.write("int " + get_word_count_func + "() {\n")
        f.write("    return word_count;\n")
        f.write("}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate C/C++ wordlist source and header files.')
    parser.add_argument('input_file', help='Path to the input wordlist file (text)')
    parser.add_argument('name', help='Name of the wordlist (used for filenames and function names)')
    parser.add_argument('--output-dir', default='.', help='Output directory (default: current directory)')

    args = parser.parse_args()
    generate_cpp_files(args.input_file, args.name, args.output_dir)
