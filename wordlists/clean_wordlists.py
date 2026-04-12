#!/usr/bin/env python3
import argparse
import hashlib
import os

def get_md5(word):
    return hashlib.md5(word.encode('utf-8')).hexdigest()

def clean_file(input_path, output_path, bad_hashes):
    count = 0
    removed = 0
    with open(input_path, 'r', encoding='utf-8') as fin, open(output_path, 'w', encoding='utf-8') as fout:
        for line in fin:
            word = line.strip()
            if not word:
                continue

            # Check if the word (case-insensitive) matches any bad hash
            if get_md5(word.lower()) in bad_hashes:
                removed += 1
                continue

            fout.write(line)
            count += 1
    return count, removed

def main():
    parser = argparse.ArgumentParser(description='Clean wordlists by removing words matching MD5 hashes of bad words.')
    parser.add_argument('--input', required=True, help='Input file or directory')
    parser.add_argument('--output', required=True, help='Output file or directory')
    parser.add_argument('--bad-words-md5', required=True, help='File containing MD5 hashes of bad words (one per line)')

    args = parser.parse_args()

    if not os.path.exists(args.bad_words_md5):
        print(f"Error: Bad words MD5 file not found: {args.bad_words_md5}")
        return

    print(f"Loading bad words MD5 hashes from {args.bad_words_md5}...")
    bad_hashes = set()
    with open(args.bad_words_md5, 'r') as f:
        for line in f:
            hash_val = line.strip().lower()
            if hash_val:
                bad_hashes.add(hash_val)
    print(f"Loaded {len(bad_hashes)} bad words hashes.")

    if os.path.isdir(args.input):
        if not os.path.exists(args.output):
            os.makedirs(args.output)

        files_to_process = [f for f in os.listdir(args.input) if f.endswith(".txt")]
        if not files_to_process:
            print(f"No .txt files found in {args.input}")
            return

        for filename in files_to_process:
            input_path = os.path.join(args.input, filename)
            output_path = os.path.join(args.output, filename)
            print(f"Processing {filename}...")
            kept, removed = clean_file(input_path, output_path, bad_hashes)
            print(f"  Done. Kept {kept} words, removed {removed} bad words.")
    else:
        # Single file
        if not os.path.exists(args.input):
            print(f"Error: Input file not found: {args.input}")
            return

        # If output is a directory, use the same filename
        output_path = args.output
        if os.path.isdir(args.output):
            output_path = os.path.join(args.output, os.path.basename(args.input))
        else:
            # Ensure parent directory exists
            parent_dir = os.path.dirname(output_path)
            if parent_dir and not os.path.exists(parent_dir):
                os.makedirs(parent_dir)

        print(f"Processing {args.input}...")
        kept, removed = clean_file(args.input, output_path, bad_hashes)
        print(f"  Done. Kept {kept} words, removed {removed} bad words.")

if __name__ == "__main__":
    main()
