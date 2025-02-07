from pathlib import Path
from pm4py import ProcessTree, discover_process_tree_inductive
from pm4py.objects.log.obj import EventLog
from pm4py.objects.process_tree.utils.generic import get_leaves
import math
import pandas as pd
import pm4py


def discover_process_tree_with_naive_label_splitting(
    event_log: pd.DataFrame,
    max_distinction: int = 3,
    noise_threshold: float = 0.25,
) -> ProcessTree:
    if max_distinction < 1:
        raise ValueError("max_distinction must be at least 1.")
    digits = int(math.log10(max_distinction)) + 1

    # Add counts to activity labels
    event_log = event_log.copy()
    if "lifecycle:transition" in event_log.columns:
        event_log.sort_values(
            [
                "case:concept:name",
                "time:timestamp",
                "concept:name",
                "lifecycle:transition",
            ]
        )
        event_log["repetition"] = event_log.groupby(
            ["case:concept:name", "concept:name", "lifecycle:transition"]
        ).cumcount()
    else:
        event_log.sort_values(["case:concept:name", "time:timestamp", "concept:name"])
        event_log["repetition"] = event_log.groupby(
            ["case:concept:name", "concept:name"]
        ).cumcount()
    event_log["repetition"] = event_log["repetition"] % max_distinction
    event_log["concept:name"] = (
        event_log["concept:name"]
        + "_"
        + event_log["repetition"].astype(str).str.zfill(digits)
    )

    process_tree = discover_process_tree_inductive(
        event_log, noise_threshold=noise_threshold
    )

    # Remove counts from process tree labels
    for leaf in get_leaves(process_tree):
        if leaf.label:
            leaf.label = leaf.label[: -(digits + 1)]

    return process_tree


def discover_process_tree(
    event_log: EventLog | pd.DataFrame, noise_threshold: float = 0.25
) -> ProcessTree:
    # Discover process tree using Inductive Miner infrequent
    return discover_process_tree_inductive(event_log, noise_threshold=noise_threshold)


xesPath = Path("data/xes").resolve()
ptmlPath = Path("data/ptml").resolve()

for xes_file in xesPath.glob("*.xes"):
    if not xes_file.is_file():
        continue

    event_log = pm4py.read_xes(str(xes_file))

    if not (ptml_file := ptmlPath / f"{xes_file.stem}.ptml").is_file():

        ptmlVariants = [
            (5, 0.0, "_hard5_pt00"),
            (5, 0.1, "_hard5_pt10"),
            (5, 0.25, "_hard5_pt25"),
            (5, 0.5, "_hard5_pt50"),
        ]
        for max_distinction, noise_threshold, file_tag in ptmlVariants:
            if (ptml_file := ptmlPath / f"{xes_file.stem}{file_tag}.ptml").is_file():
                continue

            process_tree = discover_process_tree_with_naive_label_splitting(
                event_log,
                max_distinction=max_distinction,
                noise_threshold=noise_threshold,
            )
            pm4py.write_ptml(
                process_tree, str(ptmlPath / f"{xes_file.stem}{file_tag}.ptml")
            )

            print(f" -> {ptml_file.stem}")

        ptmlVariants = [
            (0.0, "_pt00"),
            (0.1, "_pt10"),
            (0.25, "_pt25"),
            (0.5, "_pt50"),
        ]
        for noise_threshold, file_tag in ptmlVariants:
            if (ptml_file := ptmlPath / f"{xes_file.stem}{file_tag}.ptml").is_file():
                continue

            process_tree = discover_process_tree(
                event_log, noise_threshold=noise_threshold
            )
            pm4py.write_ptml(
                process_tree, str(ptmlPath / f"{xes_file.stem}{file_tag}.ptml")
            )
        print(f" -> {ptml_file.stem}")
