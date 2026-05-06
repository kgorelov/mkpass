#!/usr/bin/env python3
import sys
import os
import gzip
import msgpack
import argparse
import unicodedata

def preprocess_en(word):
    # For English, wordfreq preprocessing is basically NFC normalization and casefolding
    return unicodedata.normalize("NFC", word).casefold().strip()

def load_freq_dict(data_path, threshold):
    """
    Load words from msgpack.gz that meet the Zipf threshold.
    Zipf = (900 - index) / 100.0
    So index = 900 - 100 * Zipf
    """
    max_index = int(900 - 100 * threshold)

    with gzip.open(data_path, 'rb') as f:
        data = msgpack.load(f, raw=False)

    # data[0] is header
    # data[1:] are buckets
    buckets = data[1:]

    word_to_zipf = {}
    # We only need to iterate up to max_index
    for index, bucket in enumerate(buckets):
        if index > max_index:
            break
        zipf = (900 - index) / 100.0
        for word in bucket:
            if word not in word_to_zipf:
                word_to_zipf[word] = zipf
    return word_to_zipf

def main():
    parser = argparse.ArgumentParser(description="Filter a wordlist by Zipf frequency using wordfreq data.")
    parser.add_argument("input_file", help="Input file (one word per line)")
    parser.add_argument("threshold", type=float, help="Minimum Zipf frequency (0.0 to 8.0). Common words are 4+, rare are <2.")
    parser.add_argument("--data", help="Path to wordfreq msgpack.gz data file (default: large_en.msgpack.gz)")

    args = parser.parse_args()

    data_file = args.data
    if not data_file:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        data_file = os.path.join(base_dir, "large_en.msgpack.gz")

    if not os.path.exists(data_file):
        print(f"Error: Data file {data_file} not found.", file=sys.stderr)
        sys.exit(1)

    print(f"Loading words with Zipf >= {args.threshold} from {data_file}...", file=sys.stderr)
    word_to_zipf = load_freq_dict(data_file, args.threshold)
    print(f"Loaded {len(word_to_zipf)} words meeting the threshold.", file=sys.stderr)

    if not os.path.exists(args.input_file):
        print(f"Error: Input file {args.input_file} not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.input_file, 'r') as f:
        for line in f:
            original_word = line.strip()
            if not original_word:
                continue

            processed_word = preprocess_en(original_word)

            if processed_word in word_to_zipf:
                print(original_word)

if __name__ == "__main__":
    main()
