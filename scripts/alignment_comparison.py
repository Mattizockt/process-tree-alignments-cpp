from python_dyn_align import dyn_align
from datetime import datetime
from pathlib import Path
from pm4py import ProcessTree, discover_process_tree_inductive
from pm4py.algo.conformance.alignments.petri_net.algorithm import __get_variants_structure as get_variants
from pm4py.objects.log.obj import EventLog
from pm4py.objects.process_tree.utils.generic import is_leaf
import alignment
import pandas as pd
import pm4py
import random
import time

def discover_process_tree(
    event_log: EventLog | pd.DataFrame, noise_threshold: float = 0.25
) -> ProcessTree:
    # Discover process tree using Inductive Miner infrequent
    return discover_process_tree_inductive(event_log, noise_threshold=noise_threshold)


def get_ids_from_process_tree(pt_node):
    if is_leaf(pt_node):
        ids = [pt_node._get_label()[1]]
    else:
        ids = [pt_node._get_label()[1]] + [
            pt_id
            for child in pt_node.children
            for pt_id in get_ids_from_process_tree(child)
        ]
    # sort the ids
    return sorted(ids)


def get_subsequence_dict(pt_node):
    # Check that pt_node is a process tree with ids
    if not type(pt_node.label) is tuple:
        raise ValueError(
            "Subsequence Dict cannot be computed: pt_node is not a process tree with ids"
        )
    # Loop through all subsequences of length at most ssl
    subseq_dict = {}
    subsequence_list = [-1] * len(get_ids_from_process_tree(pt_node))
    for pt_id in get_ids_from_process_tree(pt_node):
        subsequence_list[pt_id] = subseq_dict.copy()
    return subsequence_list


def get_letter_dict_of_process_tree(pt_node, d={}):
    label, pt_node_id = pt_node._get_label()
    if pt_node.operator is None:
        d[pt_node_id] = {label} if label is not None else set()
    else:
        letters = set()
        for child in pt_node.children:
            _, child_node_id = child._get_label()
            d = d | get_letter_dict_of_process_tree(child, d)
            letters = letters.union(d[child_node_id])
        d[pt_node_id] = letters
    return d


def add_id_to_process_tree(pt_node, counter=0):
    """
    This function adds an id to each node in the process tree
    returns maximum counter value from subtree
    """
    old_label = pt_node._get_label()
    if type(old_label) is tuple:
        return counter
    pt_node._set_label((old_label, counter))

    # Case of n-ary operators:
    if pt_node.operator is not None:
        for child in pt_node.children:
            counter = add_id_to_process_tree(child, counter + 1)
    return counter


current_dir = Path.cwd()
print(current_dir)
xes_path = current_dir / "data" / "xes"
ptml_path = current_dir / "data" / "ptml"

result_path = Path("output") / datetime.now().strftime("%Y%m%d%H%M%S")
result_path.mkdir()

evaluate_event_logs = []
for xes_file in xes_path.glob("*.xes"):
    if not xes_file.is_file():
        continue

    cur_path = result_path / xes_file.stem
    cur_path.mkdir()
    print(f"{xes_file.stem}")
    event_log = pm4py.read_xes(str(xes_file))

    for noise_threshold, file_tag in [(0.5, "_pt50")]:
        if (ptml_file := ptml_path / f"{xes_file.stem}{file_tag}.ptml").is_file():
            process_tree = pm4py.read_ptml(str(ptml_file))
            process_tree_with_ids = pm4py.read_ptml(str(ptml_file))
        else:
            process_tree = discover_process_tree(
                event_log, noise_threshold=noise_threshold
            )
            pm4py.write_ptml(
                process_tree, str(ptml_path / f"{xes_file.stem}{file_tag}.ptml")
            )
            process_tree_with_ids = pm4py.read_ptml(
                str(ptml_path / f"{xes_file.stem}{file_tag}.ptml")
            )
        print(f"adding -> {ptml_file.stem}")
        evaluate_event_logs.append(
            {
                "event_log": event_log,
                "process_tree": process_tree,
                "process_tree_with_ids": process_tree_with_ids,
                "repeat": 1,
                "result_path": cur_path,
                "file_tag": file_tag,
            }
        )


aligner = alignment.AlignmentWrapper()
aligner.loadTree(str(ptml_path / "BPI_Challenge_2012_pt50.ptml"))
rg = random.Random(12345)

for benchmark in evaluate_event_logs:
    add_id_to_process_tree(benchmark["process_tree_with_ids"])
    letters_dict = get_letter_dict_of_process_tree(benchmark["process_tree_with_ids"])
    trace_variants = list(zip(*get_variants(benchmark["event_log"], None)))
    rg.shuffle(trace_variants)
    
    subsequence_dict = get_subsequence_dict(benchmark["process_tree_with_ids"])
    
    for variant, trace in trace_variants:
        trace_as_list = tuple([event["concept:name"] for event in trace])
        aligner.setTrace(list(trace_as_list))

        cpp_start = time.time()
        cpp_cost = aligner.align()
        cpp_end = time.time()

        py_start = time.time()
        cost = dyn_align(
            benchmark["process_tree_with_ids"],
            letters_dict,
            trace_as_list,
            subsequence_dict,
        )
        py_end = time.time()

        cpp_duration  = cpp_end - cpp_start
        py_duration = py_end - py_start

        with open(result_path / "costs.csv", 'a') as file:
            file.writelines(f"{cpp_cost}, {cpp_duration}, {cost}, {py_duration}, {trace_as_list} \n")
