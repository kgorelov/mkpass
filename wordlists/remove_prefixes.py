#!/usr/bin/env python3
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: remove_prefixes.py <input_file>", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]

    with open(input_file, 'r') as f:
        # Assuming the wordlist is sorted
        words = [line.strip() for line in f if line.strip()]

    if not words:
        return

    # To efficiently remove prefix words from a sorted list,
    # we just need to check if words[i] is a prefix of words[i+1].

    prefix_free_words = []
    for i in range(len(words) - 1):
        # If the current word is not a prefix of the next word, keep it
        if not words[i+1].startswith(words[i]):
            prefix_free_words.append(words[i])

    # The last word is never a prefix of a following word
    prefix_free_words.append(words[-1])

    for word in prefix_free_words:
        print(word)

if __name__ == "__main__":
    main()
