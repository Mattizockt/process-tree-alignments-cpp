from datetime import datetime
from pathlib import Path
from pm4py import ProcessTree, discover_process_tree_inductive
from pm4py.algo.conformance.alignments.petri_net.algorithm import (
    __get_variants_structure as get_variants,
)
from pm4py.objects.log.obj import EventLog
from pm4py.objects.process_tree.utils.generic import is_leaf
from python_dyn_align import dyn_align
import alignment
from multiprocessing import Process, Queue
import pandas as pd
import pm4py
import random
import time
import random

def dyn_align_wrapper(benchmark, letters_dict, trace_as_list, subsequence_dict, result_queue):
    result = dyn_align(benchmark, letters_dict, trace_as_list, subsequence_dict)
    result_queue.put(result)

class ProcessTreeManager:
    @staticmethod
    def discover_process_tree(
        event_log: EventLog | pd.DataFrame, noise_threshold: float = 0.25
    ) -> ProcessTree:
        return discover_process_tree_inductive(
            event_log, noise_threshold=noise_threshold
        )

    @staticmethod
    def add_id_to_process_tree(pt_node, counter=0):
        old_label = pt_node._get_label()
        if isinstance(old_label, tuple):
            return counter

        pt_node._set_label((old_label, counter))

        if pt_node.operator is not None:
            for child in pt_node.children:
                counter = ProcessTreeManager.add_id_to_process_tree(child, counter + 1)
        return counter

    @staticmethod
    def get_ids_from_process_tree(pt_node):
        if is_leaf(pt_node):
            ids = [pt_node._get_label()[1]]
        else:
            ids = [pt_node._get_label()[1]] + [
                pt_id
                for child in pt_node.children
                for pt_id in ProcessTreeManager.get_ids_from_process_tree(child)
            ]
        return sorted(ids)

    @staticmethod
    def get_subsequence_dict(pt_node):
        if not isinstance(pt_node.label, tuple):
            raise ValueError(
                "Subsequence Dict cannot be computed: pt_node is not a process tree with IDs"
            )

        subseq_dict = {}
        all_ids = ProcessTreeManager.get_ids_from_process_tree(pt_node)
        subsequence_list = [-1] * len(all_ids)

        for pt_id in all_ids:
            subsequence_list[pt_id] = subseq_dict.copy()

        return subsequence_list

    @staticmethod
    def get_letter_dict_of_process_tree(pt_node, d=None):
        if d is None:
            d = {}

        label, pt_node_id = pt_node._get_label()

        if pt_node.operator is None:
            d[pt_node_id] = {label} if label is not None else set()
        else:
            letters = set()
            for child in pt_node.children:
                _, child_node_id = child._get_label()
                d = d | ProcessTreeManager.get_letter_dict_of_process_tree(child, d)
                letters = letters.union(d[child_node_id])
            d[pt_node_id] = letters

        return d


class AlignmentEvaluator:
    def __init__(self):
        self.aligner = alignment.AlignmentWrapper()
        self.random_generator = random.Random(12345)
    
    def load_tree(self, treeString):
        self.aligner.loadTree(treeString)

    def compare_alignments(self, trace_as_list, repeats=1):
        min_cpp_dur = float("inf")
        min_py_dur = float("inf")
        for _ in range(repeats):
            cpp_start = time.time()
            cpp_cost = self.aligner.align(trace_as_list)
            cpp_end = time.time()
            cpp_duration = cpp_end - cpp_start
            min_cpp_dur = min(min_cpp_dur, cpp_duration)

            result_queue = Queue()
            p = Process(
                target=dyn_align_wrapper,
                args=(
                    self.benchmark["process_tree_with_ids"],
                    self.letters_dict,
                    tuple(trace_as_list),
                    self.subsequence_dict,
                    result_queue
                ),
            )
            py_start = time.time()
            p.start()
            p.join(120)
            py_end = time.time()
            if p.is_alive():
                p.terminate()
                p.join()
                py_cost = -1
                print("errror")
            else:
                try:
                    py_cost = result_queue.get(timeout=5)
                except result_queue.empty():
                    print("error when retrieving alignment result")
                    py_cost = -1
            
            p.close()
            py_duration = py_end - py_start
            min_py_dur = min(min_py_dur, py_duration)

            return {
                "cpp_cost": py_cost,
                "cpp_duration": min_py_dur,
                "cpp_cost": cpp_cost,
                "cpp_duration": min_cpp_dur,
                "py_cost": py_cost,
                "py_duration": min_py_dur,
                "trace": trace_as_list,
            }

    def run_evaluation(self, benchmark, result_path):
        self.benchmark = benchmark
        ProcessTreeManager.add_id_to_process_tree(benchmark["process_tree_with_ids"])
        self.letters_dict = ProcessTreeManager.get_letter_dict_of_process_tree(
            benchmark["process_tree_with_ids"]
        )
        self.subsequence_dict = ProcessTreeManager.get_subsequence_dict(
            benchmark["process_tree_with_ids"]
        )

        output_path = result_path / benchmark["tree_name"]
        output_path.mkdir(exist_ok=True)
        results = []
        for trace in benchmark["event_log"]:
            result = self.compare_alignments(trace, benchmark["repeat"])
            results.append(result)

            with open(output_path / "costs.csv", "a") as file:
                file.write(
                    f"{result['cpp_cost']}, {result['cpp_duration']}, "
                    f"{result['py_cost']}, {result['py_duration']},"
                    f"{result['trace']}\n"
                )

        return results


class DataManager:
    def __init__(self):
        random.seed(123)
        self.current_dir = Path.cwd()
        self.xes_path = self.current_dir / "data" / "xes"
        self.ptml_path = self.current_dir / "data" / "ptml"
        self.result_path = Path("output") / datetime.now().strftime("%Y%m%d%H%M%S")
        self.result_path.mkdir(exist_ok=True)

    def load_event_logs(self, num_traces=1000):
        evaluate_event_logs = []

        for xes_file in self.xes_path.glob("*.xes"):
            if not xes_file.is_file():
                continue

            cur_path = self.result_path / xes_file.stem
            cur_path.mkdir(exist_ok=True)
            print(f"Processing {xes_file.stem}")

            unparsed_event_logs = pm4py.read_xes(str(xes_file))
            unsampled_event_logs = [
                x[0] for x in zip(*get_variants(unparsed_event_logs, None))
            ]
            tuple_event_logs = random.sample(unsampled_event_logs, num_traces)
            event_logs = [list(x) for x in tuple_event_logs]

            for noise_threshold, file_tag in [
                (0.0, "_pt00"),
                (0.1, "_pt10"),
                (0.25, "_pt25"),
                (0.5, "_pt50"),
            ]:
                ptml_file = self.ptml_path / f"{xes_file.stem}{file_tag}.ptml"

                if ptml_file.is_file():
                    process_tree = pm4py.read_ptml(str(ptml_file))
                    process_tree_with_ids = pm4py.read_ptml(str(ptml_file))
                else:
                    process_tree = ProcessTreeManager.discover_process_tree(
                        event_logs, noise_threshold=noise_threshold
                    )
                    pm4py.write_ptml(
                        process_tree,
                        str(self.ptml_path / f"{xes_file.stem}{file_tag}.ptml"),
                    )
                    process_tree_with_ids = pm4py.read_ptml(
                        str(self.ptml_path / f"{xes_file.stem}{file_tag}.ptml")
                    )

                print(f"Adding benchmark: {ptml_file.stem}")
                evaluate_event_logs.append(
                    {
                        "event_log": event_logs,
                        "process_tree": process_tree,
                        "process_tree_with_ids": process_tree_with_ids,
                        "repeat": 5,
                        "result_path": cur_path,
                        "file_tag": file_tag,
                        "tree_name": f"{xes_file.stem}{file_tag}",
                    }
                )

        return evaluate_event_logs


if __name__ == "__main__":
    data_manager = DataManager()
    evaluate_event_logs = data_manager.load_event_logs()

    evaluator = AlignmentEvaluator()

    for i, benchmark in enumerate(evaluate_event_logs):
        evaluator.load_tree(str(benchmark["process_tree"]))
        evaluator.run_evaluation(benchmark, data_manager.result_path)