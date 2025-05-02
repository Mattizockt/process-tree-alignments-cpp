import csv
from collections import defaultdict
import sys

def analyze_words_in_csv(file_path):
    """
    Reads a CSV file, counts occurrences of each unique word (cell content)
    from only the word-containing lines (skipping numerical lines),
    and returns a dictionary of word counts and the number of word lines processed.
    Treats each cell's content after stripping whitespace as a 'word'.
    Assumes word lines are the first, third, fifth, etc. rows (0-indexed: 0, 2, 4...).
    """
    word_counts = defaultdict(int) # Use defaultdict for easier counting
    total_word_lines_processed = 0
    row_index = 0 # Keep track of the physical row number

    try:
        with open(file_path, "r", newline="", encoding="utf-8") as csvfile:
            csv_reader = csv.reader(csvfile)
            # Optional: Skip header row if necessary (adjust row_index if skipping)
            # try:
            #     next(csv_reader)
            #     row_index += 1
            # except StopIteration:
            #     pass # File was empty or only had header

            for row in csv_reader:
                # Process only rows where index is even (0, 2, 4, ...)
                # This corresponds to the 1st, 3rd, 5th, ... physical lines
                if row_index % 2 == 0:
                    total_word_lines_processed += 1
                    # Iterate through each cell in the row
                    for cell in row:
                        word = cell.strip() # Clean the word (cell content)
                        if word: # Only count non-empty strings after stripping
                            word_counts[word] += 1

                row_index += 1 # Always increment the physical row index

        # Return the counts dictionary and the count of word lines processed
        return dict(word_counts), total_word_lines_processed # Convert defaultdict back to dict

    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
        return None, 0  # Indicate error
    except Exception as e:
        print(f"An error occurred while processing '{file_path}': {e}")
        return None, 0  # Indicate error

# Helper function to process and print results for a single file
def process_and_print_file_analysis(file_path):
    """
    Analyzes the given file (skipping numerical lines), prints the number
    of word-containing lines processed, and then lists occurrences and
    average per word line for every unique word found.
    """
    print(f"Processing file: '{file_path}'")
    word_counts, total_word_lines_processed = analyze_words_in_csv(file_path)

    if word_counts is not None: # Check if analysis was successful (not None from error)
        print(f"Total word-containing lines processed: {total_word_lines_processed}")

        if total_word_lines_processed > 0:
            print("\n--- Word Analysis ---")
            # Sort words alphabetically for consistent output
            sorted_words = sorted(word_counts.keys())

            for word in sorted_words:
                count = word_counts[word]
                # Calculate average based on the number of word lines processed
                average = count / total_word_lines_processed
                # Print formatted results
                print(f"Word: '{word}' | Occurrences: {count} | Average per word line: {average:.4f}")
            print("---------------------\n") # Separator after word list
        else:
            print("No word-containing lines processed or no words found in the file.\n")
    else:
        # Error message was already printed by analyze_words_in_csv
        print("Skipping analysis due to previous error.\n")

def analyze_numbers_in_bins(file_path):
    """
    Reads numbers from a file (one per line) and categorizes them into specified bins.

    Args:
        file_path (str): The path to the input file.
    """
    # Define the bins as a dictionary to store counts
    # Using strings for keys to represent the bin ranges clearly
    bins = {
        '0': 0,          # Exactly 0
        '1': 0,          # Exactly 1
        '0.9 - <1.0': 0,
        '0.8 - <0.9': 0,
        '0.7 - <0.8': 0,
        '0.6 - <0.7': 0,
        '0.5 - <0.6': 0,
        '0.4 - <0.5': 0,
        '0.3 - <0.4': 0,
        '0.2 - <0.3': 0,
        '0.1 - <0.2': 0,
        '0.0 - <0.1': 0  # Numbers greater than 0 and less than 0.1
    }

    total_numbers_read = 0
    errors = []

    try:
        with open(file_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line: # Skip empty lines
                    continue

                try:
                    number = float(line)

                    # Check if the number is within the expected range [0, 1]
                    if 0 <= number <= 1:
                        total_numbers_read += 1
                        # Categorize the number into bins
                        if number == 0:
                            bins['0'] += 1
                        elif number == 1:
                            bins['1'] += 1
                        elif 0.9 <= number < 1.0:
                            bins['0.9 - <1.0'] += 1
                        elif 0.8 <= number < 0.9:
                            bins['0.8 - <0.9'] += 1
                        elif 0.7 <= number < 0.8:
                            bins['0.7 - <0.8'] += 1
                        elif 0.6 <= number < 0.7:
                            bins['0.6 - <0.7'] += 1
                        elif 0.5 <= number < 0.6:
                            bins['0.5 - <0.6'] += 1
                        elif 0.4 <= number < 0.5:
                            bins['0.4 - <0.5'] += 1
                        elif 0.3 <= number < 0.4:
                            bins['0.3 - <0.4'] += 1
                        elif 0.2 <= number < 0.3:
                            bins['0.2 - <0.3'] += 1
                        elif 0.1 <= number < 0.2:
                            bins['0.1 - <0.2'] += 1
                        elif 0.0 < number < 0.1: # Numbers greater than 0 and less than 0.1
                             bins['0.0 - <0.1'] += 1
                        # Note: Numbers exactly 0.0 are caught by the first condition (number == 0)
                        # Note: Numbers exactly 1.0 are caught by the second condition (number == 1)
                    else:
                        pass
                        # errors.append(f"Line {line_num}: Number {number} is outside the expected range [0, 1].")

                except ValueError:
                    errors.append(f"Line {line_num}: Could not convert '{line}' to a float.")

    except FileNotFoundError:
        print(f"Error: File not found at '{file_path}'")
        return
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return

    # Print the results
    print("\n--- Bin Analysis Results ---")
    print(f"Total valid numbers read: {total_numbers_read}")
    print("\nBin Counts:")
    # Print bins in a logical order
    print(f"  Exactly 0:        {bins['0']}")
    print(f"  0.0 - <0.1:      {bins['0.0 - <0.1']}")
    print(f"  0.1 - <0.2:      {bins['0.1 - <0.2']}")
    print(f"  0.2 - <0.3:      {bins['0.2 - <0.3']}")
    print(f"  0.3 - <0.4:      {bins['0.3 - <0.4']}")
    print(f"  0.4 - <0.5:      {bins['0.4 - <0.5']}")
    print(f"  0.5 - <0.6:      {bins['0.5 - <0.6']}")
    print(f"  0.6 - <0.7:      {bins['0.6 - <0.7']}")
    print(f"  0.7 - <0.8:      {bins['0.7 - <0.8']}")
    print(f"  0.8 - <0.9:      {bins['0.8 - <0.9']}")
    print(f"  0.9 - <1.0:      {bins['0.9 - <1.0']}")
    print(f"  Exactly 1:        {bins['1']}")
    print(f"Number of not perfectly fitting bins is: {total_numbers_read - bins['1']}")

    if errors:
        print("\n--- Errors Encountered ---")
        for error in errors:
            print(error)

path = "outliers_aliens.txt"
analyze_numbers_in_bins(path)
# # Example usage:
# normal_path = "trace_counter_normal.csv"
# outlier_path = "trace_counter_outlier.csv"

# # Process the normal file
# process_and_print_file_analysis(normal_path)

# # Process the outlier file
# process_and_print_file_analysis(outlier_path)



# total 67762511 + 9677985
# not perfectly fitting 23905449 + 3554038