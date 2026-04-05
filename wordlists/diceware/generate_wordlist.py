import os

def generate_cpp_files(input_file, header_file, source_file):
    with open(input_file, 'r') as f:
        words = [line.strip() for line in f if line.strip()]

    blob = ""
    offsets = []
    current_offset = 0

    for word in words:
        offsets.append(current_offset)
        blob += word + "\0"
        current_offset += len(word) + 1

    # Header file
    with open(header_file, 'w') as f:
        f.write("#ifndef EFF_LARGE_WORDS_H\n")
        f.write("#define EFF_LARGE_WORDS_H\n\n")
        f.write("#include <string>\n\n")
        f.write("const char* eff_large_get_word(int idx);\n")
        f.write("int eff_large_get_word_count();\n\n")
        f.write("#endif // EFF_LARGE_WORDS_H\n")

    # Source file
    with open(source_file, 'w') as f:
        f.write("#include \"" + header_file + "\"\n")
        f.write("#include <stdexcept>\n\n")
        
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

        f.write("const char* eff_large_get_word(int idx) {\n")
        f.write("    if (idx < 0 || idx >= word_count) return nullptr;\n")
        f.write("    return words_blob + word_offsets[idx];\n")
        f.write("}\n\n")
        
        f.write("int eff_large_get_word_count() {\n")
        f.write("    return word_count;\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_cpp_files('eff_large_words.txt', 'eff_large_words.h', 'eff_large_words.cpp')
