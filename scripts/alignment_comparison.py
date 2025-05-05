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
import re
import csv
import pandas as pd
import pm4py
import random
import time
import json


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
    def __init__(self, ptml_path):
        self.aligner = alignment.AlignmentWrapper()
        self.aligner.loadTree(str(ptml_path / "BPI_Challenge_2012_pt50.ptml"))
        self.random_generator = random.Random(12345)

    def compare_cpp_alignments(self, trace_as_list, repeats=1):
        self.aligner.setTrace(list(trace_as_list))

        min_cpp_dur = float("inf")
        for i in range(repeats):
            cpp_start = time.time()
            cpp_cost = self.aligner.align()
            cpp_end = time.time()
            cpp_duration = cpp_end - cpp_start
            min_cpp_dur = min(min_cpp_dur, cpp_duration)

        return {
            "cpp_cost": cpp_cost,
            "cpp_duration": min_cpp_dur,
            "trace": trace_as_list,
        }

    def compare_alignments(self, trace_as_list, repeats=1):
        self.aligner.setTrace(list(trace_as_list))

        min_cpp_dur = float("inf")
        # min_py_dur = float("inf")
        for _ in range(repeats):
            cpp_start = time.time()
            cpp_cost = self.aligner.align()
            cpp_end = time.time()
            cpp_duration = cpp_end - cpp_start
            min_cpp_dur = min(min_cpp_dur, cpp_duration)

            # py_start = time.time()
            # py_cost = dyn_align(
            #     self.benchmark["process_tree_with_ids"],
            #     self.letters_dict,
            #     trace_as_list,
            #     self.subsequence_dict,
            # )
            # py_end = time.time()
            # py_duration = py_end - py_start
            # min_py_dur = min(min_py_dur, py_duration)

        return {
            "cpp_cost": cpp_cost,
            "cpp_duration": min_cpp_dur,
            # "py_cost": py_cost,
            # "py_duration": min_py_dur,
            "trace": trace_as_list,
        }

    def run_cpp_evaluation(self, benchmark, result_path):
        self.benchmark = benchmark
        # ProcessTreeManager.add_id_to_process_tree(benchmark["process_tree_with_ids"])
        result = self.compare_cpp_alignments(benchmark["event_log"], 5)

        with open(result_path / "costs.csv", "a") as file:
            file.write(
                f"{result['cpp_cost']}, {result['cpp_duration']}, "
                f"{result['trace']}\n"
            )

    def run_evaluation(self, benchmark, result_path):
        self.benchmark = benchmark
        ProcessTreeManager.add_id_to_process_tree(benchmark["process_tree_with_ids"])
        self.letters_dict = ProcessTreeManager.get_letter_dict_of_process_tree(
            benchmark["process_tree_with_ids"]
        )
        self.subsequence_dict = ProcessTreeManager.get_subsequence_dict(
            benchmark["process_tree_with_ids"]
        )

        trace_variants = list(zip(*get_variants(benchmark["event_log"], None)))
        self.random_generator.shuffle(trace_variants)

        results = []
        for variant, trace in trace_variants:
            trace_as_list = tuple([event["concept:name"] for event in trace])
            result = self.compare_alignments(trace_as_list, 2)
            results.append(result)

            with open(result_path / "costs.csv", "a") as file:
                file.write(
                    f"{result['cpp_cost']}, {result['cpp_duration']}, "
                    # f"{result['py_cost']}, {result['py_duration']},
                    f"{result['trace']}\n"
                )

        return results


class DataManager:
    def __init__(self):
        self.current_dir = Path.cwd()
        self.xes_path = self.current_dir / "data" / "xes"
        self.ptml_path = self.current_dir / "data" / "ptml"
        self.result_path = Path("output") / datetime.now().strftime("%Y%m%d%H%M%S")
        self.result_path.mkdir(exist_ok=True)

    def parse_event_csv(self, file_path):
        results = []

        with open(file_path, "r") as file:
            for line in file:
                if not line.strip():
                    continue

                parts = line.split(
                    ",", 4
                )  # Split by comma, but only for the first 4 commas

                tuple_part = parts[4].strip()
                events = re.findall(r"'([^']*)'", tuple_part)

                results.append(events)

        return results

    def load_special_event_logs(self):
        evaluate_event_logs = []
        path = (
            "/home/matthias/rwth/ba/process-tree-alignments-cpp/data/outliers/main.csv"
        )
        result_path = self.result_path / "outliers"
        event_logs = list(self.parse_event_csv(path))

        for noise_threshold, file_tag in [(0.5, "_pt50")]:
            ptml_file = Path(
                "/home/matthias/rwth/ba/process-tree-alignments-cpp/data/ptml/BPI_Challenge_2012_pt50.ptml"
            )

            process_tree = pm4py.read_ptml(str(ptml_file))
            process_tree_with_ids = pm4py.read_ptml(str(ptml_file))

            print(f"Adding benchmark: {ptml_file.stem}")
            for event_log in event_logs:
                evaluate_event_logs.append(
                    {
                        "event_log": event_log,
                        "process_tree": process_tree,
                        "process_tree_with_ids": process_tree_with_ids,
                        "repeat": 5,
                        "result_path": result_path,
                        "file_tag": file_tag,
                    }
                )

        return evaluate_event_logs

    def load_event_logs(self):
        evaluate_event_logs = []

        for xes_file in self.xes_path.glob("*.xes"):
            if not xes_file.is_file():
                continue

            cur_path = self.result_path / xes_file.stem
            cur_path.mkdir(exist_ok=True)
            print(f"Processing {xes_file.stem}")

            event_log = pm4py.read_xes(str(xes_file))
            for noise_threshold, file_tag in [
                (0, "_pt00"),
                (0, "_pt10"),
                (0, "_pt25"),
                (0.5, "_pt50"),
            ]:
                ptml_file = self.ptml_path / f"{xes_file.stem}{file_tag}.ptml"

                if ptml_file.is_file():
                    process_tree = pm4py.read_ptml(str(ptml_file))
                    process_tree_with_ids = pm4py.read_ptml(str(ptml_file))
                else:
                    process_tree = ProcessTreeManager.discover_process_tree(
                        event_log, noise_threshold=noise_threshold
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
                        "event_log": event_log,
                        "process_tree": process_tree,
                        "process_tree_with_ids": process_tree_with_ids,
                        "repeat": 3,
                        "result_path": cur_path,
                        "file_tag": file_tag,
                    }
                )

        return evaluate_event_logs


def calculate_percentage_of_max(file_path):
    try:
        with open(file_path, "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found at {file_path}", file=sys.stderr)
        return None
    except json.JSONDecodeError:
        print(
            f"Error: Could not decode JSON from {file_path}. Please ensure it's valid JSON.",
            file=sys.stderr,
        )
        return None
    except Exception as e:
        print(
            f"An unexpected error occurred while reading the file: {e}", file=sys.stderr
        )
        return None

    if not isinstance(data, dict):
        print(
            f"Error: The content of {file_path} is not a JSON object (dictionary).",
            file=sys.stderr,
        )
        return {}  # Return empty dictionary if not a dict

    # Filter out non-numeric values just in case, although the example shows numbers
    numeric_values = [v for v in data.values() if isinstance(v, (int, float))]

    if not numeric_values:
        print("No numeric values found in the dictionary.", file=sys.stderr)
        return {
            key: 0.0 for key in data.keys()
        }  # Return 0% for all if no max can be determined

    max_time = max(numeric_values)

    if max_time == 0:
        # Avoid division by zero if the maximum value is 0
        percentage_data = {key: 0.0 for key in data.keys()}
    else:
        # Calculate percentages
        percentage_data = {}
        for key, value in data.items():
            if isinstance(value, (int, float)):
                percentage_data[key] = (value / max_time) * 100
            else:
                # Handle keys with non-numeric values if necessary, setting percentage to 0 or similar
                percentage_data[key] = 0.0  # Or handle as appropriate

    return percentage_data


def main():
    data_manager = DataManager()
    # evaluate_event_logs = data_manager.load_special_event_logs()
    evaluate_event_logs = data_manager.load_event_logs()

    # loads 50 tree at the mom
    evaluator = AlignmentEvaluator(data_manager.ptml_path)

    for benchmark in evaluate_event_logs:
        evaluator.run_evaluation(benchmark, data_manager.result_path)
        # evaluator.run_cpp_evaluation(benchmark, data_manager.result_path)


if __name__ == "__main__":
    main()
