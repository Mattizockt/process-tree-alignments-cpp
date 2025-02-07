import json
from pathlib import Path
import pm4py
from pm4py.objects.conversion.process_tree.converter import apply as process_tree_to_petri_net
from process_tree_graph import ProcessTreeGraph
from pm4py.algo.conformance.alignments.petri_net.algorithm import apply as pm4py_align_petri_net, __get_variants_structure as get_variants
from pm4py.algo.conformance.alignments.process_tree.algorithm import apply as pm4py_align_process_tree

# Load existing JSON if available
output_file = Path("output/alignCost.json").resolve()
try:
    with open(output_file, "r", encoding="utf-8") as file:
        costs = json.load(file)  # Load existing data
except (FileNotFoundError, json.JSONDecodeError):
    costs = {}  # Initialize if file doesn't exist or is corrupted

# Define Paths
ptmlPath = Path("data/ptml").resolve()
xesPath = Path("data/xes").resolve()

for xes_file in xesPath.glob("*.xes"):
    if not xes_file.is_file():
        continue
    print(f"Processing: {xes_file.stem}")
    
    event_log = pm4py.read_xes(str(xes_file))
    trace_variants = list(zip(*get_variants(event_log, None)))

    ptml_files = [f"{xes_file.stem}{tag}.ptml" for tag in [
        "_hard5_pt00", "_hard5_pt10", "_hard5_pt25", "_hard5_pt50",
        "_pt00", "_pt10", "_pt25", "_pt50"
    ] if (ptmlPath / f"{xes_file.stem}{tag}.ptml").is_file()]

    for ptml_file in ptml_files:
        process_tree = pm4py.read_ptml(str(ptmlPath / ptml_file))
        process_tree_graph = ProcessTreeGraph(process_tree)
        accepting_petri_net = process_tree_to_petri_net(process_tree)

        for i in range(len(trace_variants)):
            # Ensure dictionary structure exists
            costs.setdefault(ptml_file, {}).setdefault(str(i), {}).setdefault("0", None)  # Preserve existing values
            costs[ptml_file][str(i)].setdefault("1", None)
            costs[ptml_file][str(i)].setdefault("2", None)

            # Add only if not already present
            if costs[ptml_file][str(i)]["1"] is None:
                petri_res = pm4py_align_petri_net(trace_variants[i][1], *accepting_petri_net)
                costs[ptml_file][str(i)]["1"] = petri_res["cost"]

            if costs[ptml_file][str(i)]["2"] is None:
                approx_res = pm4py_align_process_tree(trace_variants[i][1], process_tree)
                costs[ptml_file][str(i)]["2"] = approx_res["cost"]

# Save updated results back to JSON
with open(output_file, "w", encoding="utf-8") as file:
    json.dump(costs, file, indent=4, ensure_ascii=False)
