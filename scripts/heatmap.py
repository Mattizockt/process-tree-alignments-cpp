import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import csv  # Using csv reader can be robust, but split works for simple cases

# --- Configuration ---
file_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/trace_counter_normal.csv"  # Make sure your data file is named aligned_data.csv
# file_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/trace_counter_outlier.csv"  # Make sure your data file is named aligned_data.csv
delimiter = ","  # The character separating values in each line
activities = ['A_SUBMITTED', ' A_PARTLYSUBMITTED', ' A_PREACCEPTED', ' W_Completeren aanvraag', ' A_ACCEPTED', ' O_SELECTED', ' A_FINALIZED', ' O_CREATED', ' O_SENT', ' W_Nabellen offertes', ' A_CANCELLED', ' O_CANCELLED', ' O_SENT_BACK', ' W_Valideren aanvraag', ' O_ACCEPTED', ' A_REGISTERED', ' A_ACTIVATED', ' A_APPROVED', ' W_Afhandelen leads', ' W_Nabellen incomplete dossiers', ' O_DECLINED', ' A_DECLINED', ' W_Beoordelen fraude']
# ---------------------actiities = ['A_SUBMITTED', ' A_PARTLYSUBMITTED', ' A_PREACCEPTED', ' W_Completeren aanvraag', ' A_ACCEPTED', ' O_SELECTED', ' A_FINALIZED', ' O_CREATED', ' O_SENT', ' W_Nabellen offertes', ' A_CANCELLED', ' O_CANCELLED', ' O_SENT_BACK', ' W_Valideren aanvraag', ' O_ACCEPTED', ' A_REGISTERED', ' A_ACTIVATED', ' A_APPROVED', ' W_Afhandelen leads', ' W_Nabellen incomplete dossiers', ' O_DECLINED', ' A_DECLINED', ' W_Beoordelen fraude']



print(f"Reading data pairs (trace, counts) from {file_path}...")

# 1. Read and parse the data pairs from the file
data_pairs = []
try:
    with open(file_path, "r") as f:
        # Read lines in pairs
        while True:
            trace_line = f.readline().strip()
            counts_line = f.readline().strip()

            if not trace_line and not counts_line:  # End of file
                break
            if not trace_line or not counts_line:  # Mismatched pair at end of file
                print(
                    f"Warning: Found a partial pair at the end of the file. Skipping last line(s)."
                )
                break

            trace = trace_line.split(delimiter)
            counts_str = counts_line.split(delimiter)

            # Convert counts to integers, handling potential errors and empty strings
            counts = []
            valid_counts = True
            if len(trace) != len(counts_str):
                print(
                    f"Warning: Skipping pair due to length mismatch. Trace length {len(trace)}, Counts length {len(counts_str)}."
                )
                print(f"Trace line: '{trace_line[:100]}...'")  # Print start of lines
                print(f"Counts line: '{counts_line[:100]}...'")
                continue  # Skip this pair

            try:
                counts = [int(c.strip()) for c in counts_str]
            except ValueError:
                print(
                    f"Warning: Skipping pair due to invalid number format in counts line: '{counts_line[:100]}...'"
                )
                continue  # Skip this pair

            if trace:  # Make sure trace is not empty
                data_pairs.append((trace, counts))

            # Optional: Print progress
            if (len(data_pairs) * 2) % 2000 == 0:  # Check every 1000 pairs
                print(f"Processed {len(data_pairs)} pairs...")


except FileNotFoundError:
    print(f"Error: File not found at {file_path}")
    print(
        "Please make sure 'aligned_data.csv' is in the same directory or provide the correct path."
    )
    exit()
except Exception as e:
    print(f"An error occurred while reading the file: {e}")
    exit()

print(f"Finished reading. Loaded {len(data_pairs)} data pairs.")

if not data_pairs:
    print(
        "No valid data pairs were loaded from the file. Please check your data file format."
    )
    exit()

# 2. Aggregate total counts and occurrence counts per (activity, position)
# Use nested dictionaries: activity_pos_agg[activity][position] = {'total': sum_of_counts, 'occurrences': count_of_times_seen}
activity_pos_agg = {}

for activity in activities:
    activity_pos_agg[activity] = {}

print("Aggregating data...")
for i, (trace, counts) in enumerate(data_pairs):
    for position, activity in enumerate(trace):
        # Make sure position is within bounds of counts list (should be true due to length check)
        if position < len(counts):
            count_value = counts[position]

            if activity not in activity_pos_agg:
                activity_pos_agg[activity] = {}
            if position not in activity_pos_agg[activity]:
                activity_pos_agg[activity][position] = {"total": 0, "occurrences": 0}

            activity_pos_agg[activity][position]["total"] += count_value
            activity_pos_agg[activity][position]["occurrences"] += 1

    # Optional: Print progress
    if (i + 1) % 1000 == 0:
        print(f"Aggregated {i + 1} pairs...")


print("Aggregation complete. Calculating average access frequencies...")

# 3. Calculate average access frequency
# Create a dictionary for the average values, then convert to DataFrame
average_frequency_data = {}

for activity, pos_data in activity_pos_agg.items():
    average_frequency_data[activity] = {}
    for position, agg_data in pos_data.items():
        total = agg_data["total"]
        occurrences = agg_data["occurrences"]
        # Calculate average = total / occurrences. Avoid division by zero if occurrences is 0
        average = (
            total / occurrences if occurrences > 0 else 0
        )  # Should always be > 0 here due to aggregation logic
        average_frequency_data[activity][position] = average

# 4. Convert average frequency data to DataFrame
average_frequency_df = pd.DataFrame.from_dict(
    average_frequency_data, orient="index"
).fillna(0)

# print(average_frequency_data.keys())

# Ensure columns cover all possible positions up to the max length found across all traces
max_overall_length = max(len(pair[0]) for pair in data_pairs) if data_pairs else 0
all_possible_positions = list(range(max_overall_length))
average_frequency_df = average_frequency_df.reindex(
    columns=all_possible_positions, fill_value=0
)

# Sort activities alphabetically for consistent Y-axis ordering
average_frequency_df = average_frequency_df.sort_index()


print("Average frequency matrix calculated. Preparing heatmap...")

# 5. Plot the heatmap
# Adjust figure size dynamically - maybe cap max size to keep it viewable
fig_width = min(max_overall_length * 0.4 + 2, 40)  # Max width 40 inches
fig_height = min(len(average_frequency_df) * 0.3 + 2, 30)  # Max height 30 inches
plt.figure(figsize=(fig_width, fig_height))

# Use the average frequency DataFrame for the heatmap
# Use a colormap suitable for numerical data (e.g., 'viridis', 'plasma', 'inferno', 'magma')
sns.heatmap(
    average_frequency_df,
    cmap="viridis",
    annot=False,
    linewidths=0.5,
    linecolor="lightgray",
)

plt.xlabel("Position in Trace")
plt.ylabel("Activity Type")
plt.title("Heatmap of Average Access Frequency by Position in Trace")
plt.tight_layout()  # Adjust layout to prevent labels overlapping

print("Showing heatmap...")
plt.show()

print("Script finished.")
