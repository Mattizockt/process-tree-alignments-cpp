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
import os

def load_data():
    xesPath = Path("data/xes").resolve()
    ptmlPath = Path("data/ptml").resolve()

    for xes_file in xesPath.glob("*.xes"):
        if not xes_file.is_file():
            continue

        event_log = pm4py.read_xes(str(xes_file))

        # create ptml files if corresponding ptml doesn't exist..
        if not (file := ptmlPath / f"{xes_file.stem}.ptml").is_file():
            ptmlVariants = [
                (0.0, "_pt00"),
                (0.1, "_pt10"),
                (0.25, "_pt25"),
                (0.5, "_pt50"),
            ]
            for noise_threshold, file_tag in ptmlVariants:
                if (ptml_file := ptmlPath / f"{xes_file.stem}{file_tag}.ptml").is_file():
                    continue

                process_tree = discover_process_tree_inductive(event_log, noise_threshold=noise_threshold)
                pm4py.write_ptml(
                    process_tree, str(ptml_file)
                )


# output_file = Path("output/alignCost.json").resolve()
def feed_compare_data(output_file : Path):
    try:
        with open(output_file, "r", encoding="utf-8") as file:
            costs = json.load(file)  # Load existing data
    except (FileNotFoundError, json.JSONDecodeError):
        costs = {}  # Initialize if file doesn't exist or is corrupted

    # Define Paths
    ptml_path = Path("data/ptml").resolve()
    xes_path = Path("data/xes").resolve()

    for xes_file in xes_path.glob("*.xes"):
        if not xes_file.is_file():
            continue
        print(f"Processing: {xes_file.stem}")

        event_log = pm4py.read_xes(str(xes_file))
        trace_variants = list(zip(*get_variants(event_log, None)))

        ptml_files = [f"{xes_file.stem}.ptml"] if (ptml_path / f"{xes_file.stem}.ptml").is_file() else [
            f"{xes_file.stem}{tag}.ptml" for tag in ["_pt00", "_pt10", "_pt25", "_pt50"]
            if (ptml_path / f"{xes_file.stem}{tag}.ptml").is_file()
        ]

        for ptml_file in ptml_files:
            process_tree = pm4py.read_ptml(str(ptml_path / ptml_file))
            # accepting_petri_net = process_tree_to_petri_net(process_tree)

            for i in range(len(trace_variants)):
                print(i)
                # Ensure dictionary structure exists
                costs.setdefault(ptml_file, {}).setdefault(str(i), {}).setdefault(
                    "adv. dyn. c++", None
                ) 
                # costs[ptml_file][str(i)].setdefault("a* petri", None)
                costs[ptml_file][str(i)].setdefault("approx", None)

                # # Add only if not already present
                # if costs[ptml_file][str(i)]["a* petri"] is None:
                #     petri_res = pm4py_align_petri_net(
                #         trace_variants[i][1], *accepting_petri_net
                #     )
                #     costs[ptml_file][str(i)]["a* petri"] = petri_res["cost"]

                if costs[ptml_file][str(i)]["approx"] is None:
                    approx_res = pm4py_align_process_tree(
                        trace_variants[i][1], process_tree
                    )
                    costs[ptml_file][str(i)]["approx"] = approx_res["cost"]

    # Save updated results back to JSON
    with open(output_file, "w", encoding="utf-8") as file:
        json.dump(costs, file, indent=4, ensure_ascii=False)

def eval_data():
    with open("output/defaultOutput.json") as f:
        data = json.load(f)
    for ptml in data:
        for trace in data[ptml]:
            if (res_1 := data[ptml][trace]["adv. dyn. c++"]) != (res_2 := data[ptml][trace]["approx"]):
                print(f"Unequal values for ptml file {ptml} for trace {trace} with adv dyn c++ = {res_1} and approx = {res_2}")


def visualizeTree():
    from pm4py.visualization.process_tree import visualizer as pt_visualizer
    # print(Path("./"))
    path = Path("./data/ptml/BPI_Challenge_2019_pt00.ptml").resolve()
    print(path)
    process_tree = pm4py.read_ptml(str(path))
    gviz = pt_visualizer.apply(process_tree)
    pt_visualizer.view(gviz)

# visualizeTree()
feed_compare_data("output.txt")
# load_data()

# os.system("/home/matthias/rwth/ba/process-tree-alignments-cpp/build/process-tree-alignments-cpp")

# output_file = Path("output/defaultOutput.json").resolve()
# feed_compare_data(output_file)

# eval_data()