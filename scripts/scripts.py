from pathlib import Path
from pm4py import discover_process_tree_inductive
import pm4py
import json
from pathlib import Path
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


# This script enhances alignment score data by adding complementary scores from different algorithms
# After C++ alignment is complete and scores are written to a file, this code adds corresponding
# A* and approximation algorithm scores for comparison purposes
def feed_compare_data(output_file: Path):
    try:
        with open(output_file, "r", encoding="utf-8") as file:
            costs = json.load(file)  # Load existing alignment scores from JSON file
    except (FileNotFoundError, json.JSONDecodeError):
        costs = (
            {}
        )  # Initialize empty dictionary if file doesn't exist or has invalid JSON format

    # Define input/output directory paths
    ptml_path = Path(
        "data/ptml"
    ).resolve()  # Path to process tree models in PTML format
    xes_path = Path("data/xes").resolve()  # Path to event logs in XES format

    for xes_file in xes_path.glob("*.xes"):
        if not xes_file.is_file():
            continue
        print(f"Processing: {xes_file.stem}")
        event_log = pm4py.read_xes(str(xes_file))  # Parse XES event log
        trace_variants = list(
            zip(*get_variants(event_log, None))
        )  # Extract trace variants from log

        # Find corresponding PTML files - either direct match or with noise level tags (_pt00, _pt10, etc.)
        ptml_files = (
            [f"{xes_file.stem}.ptml"]
            if (ptml_path / f"{xes_file.stem}.ptml").is_file()
            else [
                f"{xes_file.stem}{tag}.ptml"
                for tag in ["_pt00", "_pt10", "_pt25", "_pt50"]
                if (ptml_path / f"{xes_file.stem}{tag}.ptml").is_file()
            ]
        )

        for ptml_file in ptml_files:
            process_tree = pm4py.read_ptml(
                str(ptml_path / ptml_file)
            )  # Read process tree model
            accepting_petri_net = process_tree_to_petri_net(
                process_tree
            )  # Convert to Petri net

            for i in range(len(trace_variants)):
                # Initialize nested dictionary structure for this variant if needed
                costs.setdefault(ptml_file, {}).setdefault(str(i), {}).setdefault(
                    "adv. dyn. c++",
                    None,  # C++ alignment score placeholder (populated externally)
                )
                costs[ptml_file][str(i)].setdefault(
                    "a* petri", None
                )  # A* algorithm score placeholder
                costs[ptml_file][str(i)].setdefault(
                    "approx", None
                )  # Approximation algorithm score placeholder

                # Calculate A* alignment score if not already present
                if costs[ptml_file][str(i)]["a* petri"] is None:
                    petri_res = pm4py_align_petri_net(
                        trace_variants[i][1], *accepting_petri_net
                    )
                    costs[ptml_file][str(i)]["a* petri"] = petri_res["cost"]

                # Calculate approximation algorithm score if not already present
                if costs[ptml_file][str(i)]["approx"] is None:
                    approx_res = pm4py_align_process_tree(
                        trace_variants[i][1], process_tree
                    )
                    costs[ptml_file][str(i)]["approx"] = approx_res["cost"]

    # Save all updated results back to JSON file with formatting
    with open(output_file, "w", encoding="utf-8") as file:
        json.dump(costs, file, indent=4, ensure_ascii=False)


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


create_ptml()
# visualize_tree("./data/ptml/BPI_Challenge_2012_pt00.ptml")
# feed_compare_data("output.txt")
