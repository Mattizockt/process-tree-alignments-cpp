from collections import Counter
from pathlib import Path
import sys
from pm4py import discover_process_tree_inductive
import csv
import json
import os
import matplotlib.pyplot as plt
import numpy as np
import pm4py
import re
import pm4py
from pm4py.objects.conversion.process_tree.converter import (
    apply as process_tree_to_petri_net,
)
from pm4py.algo.conformance.alignments.petri_net.algorithm import (
    apply as pm4py_align_petri_net,
    __get_variants_structure as get_variants,
)
from pm4py.algo.conformance.alignments.process_tree.algorithm import (
    apply as pm4py_align_process_tree,
)


def get_node_depths_from_file(file_path):
    node_depths = {}

    if not os.path.exists(file_path):
        print(f"Error: File not found at {file_path}")
        return None

    try:
        with open(file_path, "r") as f:
            for line in f:
                # Calculate depth based on leading spaces. Each two spaces is one level deeper.
                leading_spaces = len(line) - len(line.lstrip(" "))
                depth = leading_spaces // 2

                # Extract the Node ID using a regular expression
                match = re.search(r"Node ID: ([0-9a-fA-F-]+)", line)
                if match:
                    node_id = match.group(1)
                    node_depths[node_id] = depth
                # No explicit warning for lines without Node ID here, assuming they might be blank or comments
                # If you need to strictly process only node lines, you can add an `else` with a warning.

    except Exception as e:
        print(f"Error reading file {file_path}: {e}")
        return None

    return node_depths


# Generate process trees (.ptml files) with different noise thresholds from a single event log (.xes)
# For each event log, this function creates 4 variations of process tree models with increasing noise tolerance
def create_ptml():
    xes_path = Path("data/xes").resolve()  # Directory containing XES event logs
    ptml_path = Path(
        "data/ptml"
    ).resolve()  # Output directory for PTML process tree models

    for xes_file in xes_path.glob("*.xes"):
        if not xes_file.is_file():
            continue
        event_log = pm4py.read_xes(str(xes_file))  # Parse the event log

        # Skip if a default process tree already exists for this event log
        if not (file := ptml_path / f"{xes_file.stem}.ptml").is_file():
            # Define noise threshold levels and corresponding filename suffixes
            ptml_variants = [
                (0.0, "_pt00"),  # No noise filtering (0%)
                (0.1, "_pt10"),  # Low noise filtering (10%)
                (0.25, "_pt25"),  # Medium noise filtering (25%)
                (0.5, "_pt50"),  # High noise filtering (50%)
            ]

            for noise_threshold, file_tag in ptml_variants:
                # Skip if this specific variant already exists
                if (
                    ptml_file := ptml_path / f"{xes_file.stem}{file_tag}.ptml"
                ).is_file():
                    continue

                # Discover process tree using inductive miner with specified noise threshold
                process_tree = discover_process_tree_inductive(
                    event_log, noise_threshold=noise_threshold
                )
                # Save the process tree to a PTML file
                pm4py.write_ptml(process_tree, str(ptml_file))


# Function to render a process tree from PTML file as a visual diagram
def visualize_tree(path="./data/ptml/BPI_Challenge_2012_pt50.ptml"):
    from pm4py.visualization.process_tree import visualizer as pt_visualizer

    # Define path to specific PTML file (BPI Challenge 2019 with 0% noise threshold)
    path = Path(path).resolve()
    print(path)  # Display the full resolved path for verification

    # Parse the PTML file into a process tree object
    process_tree = pm4py.read_ptml(str(path))

    # Convert the process tree to a GraphViz visualization object
    gviz = pt_visualizer.apply(process_tree)

    # Open the visualization in the default viewer application
    pt_visualizer.view(gviz)


def compare_output(paths: list[str], numData: int = 35):

    dicts = [dict() for _ in range(len(paths))]

    # summarize the costs
    for i, path in enumerate(paths):
        with open(path, "r", newline="", encoding="utf-8") as file:
            csv_reader = csv.reader(
                file,
                delimiter=",",
                skipinitialspace=True,
                quotechar='"',
                doublequote=True,
            )
            for j, row in enumerate(csv_reader):
                if j == numData:
                    break

                dicts[i][j] = row[0]
                # Make sure to convert string to float for plotting

    # compare for errors

    for i in range(len(dicts[0])):
        for j in range(len(dicts)):
            if dicts[j][i] != dicts[0][i]:
                print(
                    f"Error in {paths[j]} at index {i}: {dicts[j][i]} != {dicts[0][i]}"
                )
                break

    print("finished comparison")


def summarize_output(path: str, numData: int = 35):
    # Initialize the figure
    plt.figure(figsize=(10, 6))

    # Colors for different lines
    colors = ["blue", "red"]

    # Initialize empty lists for the two graphs
    graph1_data = []
    graph2_data = []

    # Read data from the file
    with open(path, "r", newline="", encoding="utf-8") as file:
        csv_reader = csv.reader(
            file,
            delimiter=",",
            skipinitialspace=True,
            quotechar='"',
            doublequote=True,
        )

        for j, row in enumerate(csv_reader):
            if j == numData:
                break

            # Extract the second and fourth values from each row for the two graphs
            # Second value (index 1) for graph 1
            graph1_data.append(float(row[1]))
            # Fourth value (index 3) for graph 2
            graph2_data.append(float(row[3]))

    # Calculate statistics for graph 1
    seconds1 = sum(graph1_data)
    minutes1 = seconds1 // 60
    minute_seconds1 = seconds1 % 60
    mean1 = seconds1 / len(graph1_data)
    std_dev1 = np.std(graph1_data)

    # Calculate statistics for graph 2
    seconds2 = sum(graph2_data)
    minutes2 = seconds2 // 60
    minute_seconds2 = seconds2 % 60
    mean2 = seconds2 / len(graph2_data)
    std_dev2 = np.std(graph2_data)

    # Print statistics for both graphs
    print(
        f"Graph 1 has the total duration of: {minutes1} minutes and {minute_seconds1:.2f} seconds"
    )
    print(f"Graph 1 has an average duration of: {mean1:.4f} seconds")
    print(f"Graph 1 has a standard deviation of: {std_dev1:.4f} seconds")
    print()
    print(
        f"Graph 2 has the total duration of: {minutes2} minutes and {minute_seconds2:.2f} seconds"
    )
    print(f"Graph 2 has an average duration of: {mean2:.4f} seconds")
    print(f"Graph 2 has a standard deviation of: {std_dev2:.4f} seconds")

    # Plot both graphs on the same figure
    x_values = np.arange(len(graph1_data))

    plt.plot(
        x_values, graph1_data, color=colors[0], linewidth=2, marker="o", label="Graph 1"
    )
    plt.plot(
        x_values, graph2_data, color=colors[1], linewidth=2, marker="o", label="Graph 2"
    )

    # Add labels and styling
    plt.xlabel("Position (i)")
    plt.ylabel("Value")
    plt.title("Comparison of Two Graphs")
    plt.grid(True, linestyle="--", alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.show()

    outlier1 = []
    outlier2 = []
    for i in range(len(graph1_data)):
        if not mean1 - std_dev1 <= graph1_data[i] <= mean1 + std_dev1:
            outlier1.append(i)
        if not mean2 - std_dev2 <= graph2_data[i] <= mean2 + std_dev2:
            outlier2.append(i)

    return outlier1, outlier2


def extract_lines(input_file, output_file, line_indices):
    line_indices = sorted(line_indices)

    with open(input_file, "r", newline="") as infile:
        reader = csv.reader(infile)
        all_rows = list(reader)
        valid_indices = [i for i in line_indices if i < len(all_rows)]
        selected_rows = [all_rows[i] for i in valid_indices]

        with open(output_file, "w", newline="") as outfile:
            writer = csv.writer(outfile)
            writer.writerows(selected_rows)

    print(f"Extracted {len(valid_indices)} lines to {output_file}")


def analyze_numbers(filename="data.txt"):
    try:
        with open(filename, "r") as f:
            numbers = [int(line.strip()) for line in f if line.strip().isdigit()]
    except FileNotFoundError:
        return {"error": f"File not found: {filename}"}
    except Exception as e:
        return {"error": f"An error occurred: {e}"}

    if not numbers:
        return {"error": "No valid numbers found in the file."}

    counts = Counter(numbers)
    total = len(numbers)

    # Create a dictionary with percentages
    percentages = {number: (count / total) * 100 for number, count in counts.items()}

    # Sort the dictionary by values (percentages) in descending order
    sorted_percentages = dict(
        sorted(percentages.items(), key=lambda item: item[1], reverse=True)
    )

    return sorted_percentages


from collections import Counter


def analyze_strings(filename="data.txt"):
    try:
        with open(filename, "r") as f:
            lines = [
                line.strip() for line in f if line.strip()
            ]  # Keep all non-empty lines as strings
    except FileNotFoundError:
        return {"error": f"File not found: {filename}"}
    except Exception as e:
        return {"error": f"An error occurred: {e}"}

    if not lines:
        return {"error": "No valid lines found in the file."}

    counts = Counter(lines)
    total = len(lines)

    # Create a dictionary with percentages
    percentages = {line: (count / total) * 100 for line, count in counts.items()}

    # Sort the dictionary by values (percentages) in descending order
    sorted_percentages = dict(
        sorted(percentages.items(), key=lambda item: item[1], reverse=True)
    )

    return sorted_percentages


def calculate_weighted_average_depth(node_depths, percentage_file_path):
    weighted_sum_of_depths = 0.0
    total_percentage = 0.0
    percentage_data = {}

    if not os.path.exists(percentage_file_path):
        print(f"Error: Percentage file not found at {percentage_file_path}")
        return None

    try:
        with open(percentage_file_path, "r") as f:
            percentage_data = json.load(f)
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON format in file {percentage_file_path}")
        return None
    except Exception as e:
        print(f"Error reading percentage file {percentage_file_path}: {e}")
        return None

    for node_id_from_percentage, percentage in percentage_data.items():
        # Determine the base node ID by stripping '_temp' if it exists
        base_node_id = re.sub(r"_temp$", "", node_id_from_percentage)

        # Check if the base node ID exists in the depth data
        if base_node_id in node_depths:
            found_depth = node_depths[base_node_id]

            # Proceed only if percentage is a valid number and non-negative
            if isinstance(percentage, (int, float)) and percentage >= 0:
                weighted_sum_of_depths += found_depth * percentage
                total_percentage += percentage
            else:
                print(
                    f"Warning: Invalid percentage value ({percentage}) for Node ID {node_id_from_percentage}. Skipping."
                )
        else:
            # This warning is based on the base_node_id not being found in depths
            print(
                f"Warning: Base Node ID '{base_node_id}' (derived from '{node_id_from_percentage}') not found in depth data. Skipping."
            )

    if total_percentage == 0:
        print(
            "Warning: Total percentage of considered nodes is zero. Cannot calculate weighted average."
        )
        return 0.0

    weighted_average = weighted_sum_of_depths / total_percentage
    return weighted_average

def sum_durations_from_file(file_path, unit="miliseconds"):
    total_durations_ns = {}

    if not os.path.exists(file_path):
        print(f"Error: File not found at {file_path}")
        return None

    try:
        with open(file_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or ":" not in line:
                    continue

                parts = line.split(":", 1)
                if len(parts) != 2:
                    print(f"Warning: Skipping improperly formatted line: {line}")
                    continue

                node_id_str = parts[0].strip()
                duration_str = parts[1].strip()

                try:
                    duration_ns = int(duration_str)
                except ValueError:
                    print(
                        f"Warning: Could not parse duration from line: {line}. Skipping."
                    )
                    continue

                if node_id_str in total_durations_ns:
                    total_durations_ns[node_id_str] += duration_ns
                else:
                    total_durations_ns[node_id_str] = duration_ns

    except Exception as e:
        print(f"Error reading file {file_path}: {e}")
        return None

    converted_durations = {}
    conversion_factor = 1

    if unit == "milliseconds":
        conversion_factor = 1_000_000
    elif unit == "seconds":
        conversion_factor = 1_000_000_000
    elif unit != "nanoseconds":
        print(f"Warning: Unknown unit '{unit}'. Returning durations in nanoseconds.")
        unit = "nanoseconds"

    for node_id, duration_ns in total_durations_ns.items():
        if unit == "nanoseconds":
            converted_durations[node_id] = duration_ns
        else:
            converted_durations[node_id] = duration_ns / conversion_factor

    return converted_durations



def calculate_percentage_of_max_and_sort(file_path):
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found at {file_path}", file=sys.stderr)
        return None
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON from {file_path}. Please ensure it's valid JSON.", file=sys.stderr)
        return None
    except Exception as e:
        print(f"An unexpected error occurred while reading the file: {e}", file=sys.stderr)
        return None

    if not isinstance(data, dict):
        print(f"Error: The content of {file_path} is not a JSON object (dictionary).", file=sys.stderr)
        return {} # Return empty dictionary if not a dict

    # Filter out non-numeric values just in case, although the example shows numbers
    numeric_values = [v for v in data.values() if isinstance(v, (int, float))]

    if not numeric_values:
        print("No numeric values found in the dictionary.", file=sys.stderr)
        # Return a dictionary with 0% for all keys if no numeric values were found
        return {key: 0.0 for key in data.keys()}

    max_time = max(numeric_values)

    percentage_data = {}
    if max_time == 0:
         # Avoid division by zero if the maximum value is 0
         percentage_data = {key: 0.0 for key in data.keys()}
    else:
         # Calculate percentages
         for key, value in data.items():
             if isinstance(value, (int, float)):
                 percentage_data[key] = (value / max_time) * 100
             else:
                 # Handle keys with non-numeric values if necessary, setting percentage to 0 or similar
                 percentage_data[key] = 0.0 # Or handle as appropriate


    sorted_items = sorted(percentage_data.items(), key=lambda item: item[1], reverse=True)

    sorted_percentage_data = dict(sorted_items)

    return sorted_percentage_data

# summarize_output(path1, numData=69)
# outlier1, outlier2 = summarize_output(path, numData=num)
# extract_lines(path, path1, outlier1)
# extract_lines(path, path2, outlier2)

# print(json.dumps(analyze_strings("out.txt"), indent=4))  # This will print with double quotes
# prints()
# print(analyze_numbers("slow.txt"))

# Read data from the file

# visualize_tree()
# print(
#     json.dumps(
#         calculate_weighted_average_depth(
#             get_node_depths_from_file(
#                 "/home/matthias/rwth/ba/process-tree-alignments-cpp/tree_depiction.txt"
#             ),
#             "/home/matthias/rwth/ba/process-tree-alignments-cpp/slow-nodes.json"
#         ),
#         indent=4,
#     )
# )


num = 3187
path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/total_comparison/costs.csv"
path1 = "/home/matthias/rwth/ba/process-tree-alignments-cpp/data/outliers/doubler.csv"
path2 = "/home/matthias/rwth/ba/process-tree-alignments-cpp/data/outliers/main.csv"


duration_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/outlier_nodes_times.json"

# print(json.dumps(sum_durations_from_file("/home/matthias/rwth/ba/process-tree-alignments-cpp/outlier_nodes.txt", unit="seconds"), indent=4))

# print(json.dumps(calculate_percentage_of_max_and_sort(duration_path), indent=4))

visualize_tree()
# length is 41.72827110134923
# length is 107.8695652173913
# length is 117.3541666666666711
