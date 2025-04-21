from pathlib import Path
from pm4py import discover_process_tree_inductive
import csv
import json
import matplotlib.pyplot as plt
import numpy as np
import pm4py
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
def visualize_tree(path="./data/ptml/BPI_Challenge_2019_pt00.ptml"):
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


# read in output and plot it on graph
def summarize_output(paths: list[str], names : list[str], numData: int = 35):
    # Initialize the figure once, outside the loop
    plt.figure(figsize=(10, 6))

    # Colors for different lines
    colors = ["blue", "red", "green", "purple", "orange"]

    # Initialize empty graphs
    graphs = [[] for _ in range(len(paths))]

    # Read data from each file
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
                # Make sure to convert string to float for plotting
                graphs[i].append(float(row[0]))

    for i in range(len(graphs)):
        miliseconds = sum(graphs[i])

        minutes = miliseconds // 60000
        minute_seconds = (miliseconds % 60000) / 1000

        seconds = miliseconds / 1000

        print(
            f"Graph {names[i]} has the total duration of : {minutes} minutes and {minute_seconds} seconds"
        )
        print(
            f"Graph {names[i]} has the an average duration of : {seconds / len(graphs[i])} seconds"
        )

    # Plot all graphs on the same figure
    for i, data in enumerate(graphs):
        x_values = np.arange(len(data))
        color = colors[i % len(colors)]
        plt.plot(
            x_values, data, color=color, linewidth=2, marker="o", label=names[i]
        )

    # Add labels and styling
    plt.xlabel("Position (i)")
    plt.ylabel("Value")
    plt.title("Multiple Graphs Visualization")
    plt.grid(True, linestyle="--", alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.show()


paths = [
    "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/sequence-improvement/BPI_Challenge_2012_pt50.ptml/times.csv",
    "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/baseline/BPI_Challenge_2012_pt50.ptml/times.csv",
    "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/upper_bounds/BPI_Challenge_2012_pt50.ptml/times.csv",
    "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/low_bound/BPI_Challenge_2012_pt50.ptml/times.csv",
    # "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/without_vector/BPI_Challenge_2012_pt50.ptml/costs.csv",
]

names = [ 
    "sequence-improvement",
    "baseline",
    "upper_bounds",
    "low_bound",
]

summarize_output(paths, names, 105)